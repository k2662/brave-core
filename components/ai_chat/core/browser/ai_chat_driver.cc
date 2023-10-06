/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_driver.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_claude.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_llama.h"
#include "brave/components/ai_chat/core/browser/models.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ui/base/l10n/l10n_util.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

namespace {
static const auto kAllowedSchemes = base::MakeFixedFlatSet<std::string_view>(
    {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme, url::kDataScheme});
}  // namespace

namespace ai_chat {

AIChatDriver::AIChatDriver(raw_ptr<PrefService> pref_service,
                           raw_ptr<AIChatMetrics> ai_chat_metrics, scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) :
      pref_service_(pref_service),
      ai_chat_metrics_(ai_chat_metrics),
      url_loader_factory_(url_loader_factory) {
  DCHECK(pref_service_);

  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kBraveChatHasSeenDisclaimer,
      base::BindRepeating(&AIChatDriver::OnUserOptedIn,
                          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kBraveChatAutoGenerateQuestions,
      base::BindRepeating(
          &AIChatDriver::OnPermissionChangedAutoGenerateQuestions,
          weak_ptr_factory_.GetWeakPtr()));

  // Engines and model names are be selectable
  // per conversation, not static.
  // Start with default.
  // TODO(petemill): Default should come from pref value.
  model_key_ = features::kAIModelKey.Get();
  InitEngine();
  DCHECK(engine_);
}

AIChatDriver::~AIChatDriver() = default;

void AIChatDriver::ChangelModel(const std::string& model_key) {
  DCHECK(!model_key.empty());
  // Check that the key exists
  if (kAllModels.find(model_key) == kAllModels.end()) {
    NOTREACHED() << "No matching model found for key: " << model_key;
    return;
  }
  model_key_ = model_key;
  InitEngine();
}

const mojom::Model& AIChatDriver::GetCurrentModel() {
  return kAllModels.find(model_key_)->second;
}

const std::vector<ConversationTurn>& AIChatDriver::GetConversationHistory() {
  return chat_history_;
}

void AIChatDriver::OnConversationActiveChanged(bool is_conversation_active) {
  is_conversation_active_ = is_conversation_active;
  DVLOG(3) << "Conversation active changed: " << is_conversation_active;
  if (MaybePopPendingRequests()) {
    return;
  }
  MaybeGeneratePageText();
  MaybeGenerateQuestions();
}

void AIChatDriver::InitEngine() {
  DCHECK(!model_key_.empty());
  auto model_match = kAllModels.find(model_key_);
  // Make sure we get a valid model, defaulting to static default or first.
  if (model_match == kAllModels.end()) {
    NOTREACHED() << "Model was not part of static model list";
    // Use default
    model_match = kAllModels.find(kModelsDefaultKey);
    const auto is_found = model_match != kAllModels.end();
    DCHECK(is_found);
    if (!is_found) {
      model_match = kAllModels.begin();
    }
  }
  auto model = model_match->second;
  // TODO(petemill): Engine enum on model to decide which one
  if (model.engine_type == mojom::ModelEngineType::LLAMA_REMOTE) {
    VLOG(1) << "Started AI engine: llama";
    engine_ = std::make_unique<EngineConsumerLlamaRemote>(
        model, url_loader_factory_);
  } else {
    VLOG(1) << "Started AI engine: claude";
    engine_ = std::make_unique<EngineConsumerClaudeRemote>(
        model, url_loader_factory_);
  }
}

bool AIChatDriver::HasUserOptedIn() {
  return pref_service_->GetBoolean(ai_chat::prefs::kBraveChatHasSeenDisclaimer);
}

void AIChatDriver::OnUserOptedIn() {
  if (!MaybePopPendingRequests()) {
    MaybeGeneratePageText();
  }

  if (ai_chat_metrics_ != nullptr && HasUserOptedIn()) {
    ai_chat_metrics_->RecordEnabled();
  }
}

void AIChatDriver::OnPermissionChangedAutoGenerateQuestions() {
  MaybeGenerateQuestions();
}

void AIChatDriver::AddToConversationHistory(mojom::ConversationTurn turn) {
  chat_history_.push_back(std::move(turn));

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }

  if (ai_chat_metrics_ != nullptr) {
    if (chat_history_.size() == 1) {
      ai_chat_metrics_->RecordNewChat();
    }

    if (turn.character_type == CharacterType::HUMAN) {
      ai_chat_metrics_->RecordNewPrompt();
    }
  }
}

void AIChatDriver::UpdateOrCreateLastAssistantEntry(std::string updated_text) {
  updated_text = base::TrimWhitespaceASCII(updated_text, base::TRIM_LEADING);
  if (chat_history_.empty() ||
      chat_history_.back().character_type != CharacterType::ASSISTANT) {
    AddToConversationHistory({CharacterType::ASSISTANT,
                              ConversationTurnVisibility::VISIBLE,
                              updated_text});
  } else {
    chat_history_.back().text = updated_text;
  }

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatDriver::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AIChatDriver::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AIChatDriver::OnFaviconImageDataChanged() {
  for (Observer& obs : observers_) {
    obs.OnFaviconImageDataChanged();
  }
}

bool AIChatDriver::MaybePopPendingRequests() {
  if (!is_conversation_active_ || !HasUserOptedIn()) {
    return false;
  }

  if (!pending_request_) {
    return false;
  }

  mojom::ConversationTurn request = std::move(*pending_request_);
  pending_request_.reset();
  MakeAPIRequestWithConversationHistoryUpdate(std::move(request));
  return true;
}

void AIChatDriver::MaybeGeneratePageText() {
  const GURL url = GetPageURL();

  if (!base::Contains(kAllowedSchemes, url.scheme())) {
    return;
  }

  // User might have already asked questions before the page is loaded. It'd be
  // strange if we generate contents based on the page.
  // TODO(sko) This makes it impossible to ask something like "Summarize this
  // page" once a user already asked a question. But for now we'd like to keep
  // it simple and not confuse users with the context changing. We'll see what
  // users say.
  if (!chat_history_.empty()) {
    return;
  }

  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  // Perf: make sure we're not doing this when the feature
  // won't be used (e.g. not opted in or no active conversation).
  if (is_page_text_fetch_in_progress_ || !article_text_.empty() ||
      !HasUserOptedIn() || !is_conversation_active_ ||
      !IsDocumentOnLoadCompletedInPrimaryMainFrame()) {
    return;
  }

  if (!HasPrimaryMainFrame()) {
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
    return;
  }

  if (!should_page_content_be_disconnected_) {
    is_page_text_fetch_in_progress_ = true;
    GetPageContent(
        base::BindOnce(&AIChatDriver::OnPageContentRetrieved,
                       weak_ptr_factory_.GetWeakPtr(), current_navigation_id_));
  }
}

void AIChatDriver::MaybeGenerateQuestions() {
  // Automatically fetch questions related to page content, if allowed
  bool can_auto_fetch_questions =
      HasUserOptedIn() && is_conversation_active_ &&
      pref_service_->GetBoolean(
          ai_chat::prefs::kBraveChatAutoGenerateQuestions) &&
      !article_text_.empty() && (suggested_questions_.size() <= 1);
  if (can_auto_fetch_questions) {
    GenerateQuestions();
  }
}

void AIChatDriver::OnPageContentRetrieved(int64_t navigation_id,
                                          std::string contents_text,
                                          bool is_video) {
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  is_page_text_fetch_in_progress_ = false;
  if (contents_text.empty()) {
    VLOG(1) << __func__ << ": No data";
    return;
  }

  // tokens + max_new_tokens must be <= 4096 (llama2)
  // 8092 chars, ~3,098 tokens (reserved for article)
  // 1k chars, ~380 tokens (reserved for prompt)
  contents_text = contents_text.substr(0, 8092);

  is_video_ = is_video;
  article_text_ = contents_text;
  engine_->SanitizeInput(article_text_);

  OnPageHasContentChanged();

  // Now that we have article text, we can suggest to summarize it
  DCHECK(suggested_questions_.empty())
      << "Expected suggested questions to be clear when there has been no"
      << " previous text content but there were " << suggested_questions_.size()
      << " suggested questions: "
      << base::JoinString(suggested_questions_, ", ");

  // Now that we have content, we can provide a summary on-demand. Add that to
  // suggested questions.
  // TODO(petemill): translation for this question
  suggested_questions_.emplace_back(is_video_ ? "Summarize this video"
                                              : "Summarize this page");
  OnSuggestedQuestionsChanged();
  MaybeGenerateQuestions();
}

void AIChatDriver::CleanUp() {
  chat_history_.clear();
  article_text_.clear();
  suggested_questions_.clear();
  pending_request_.reset();
  is_page_text_fetch_in_progress_ = false;
  is_request_in_progress_ = false;
  has_generated_questions_ = false;
  should_page_content_be_disconnected_ = false;
  OnSuggestedQuestionsChanged();
  SetAPIError(mojom::APIError::None);
  engine_->ClearAllQueries();

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnPageHasContent();
  }
}

int64_t AIChatDriver::GetNavigationId() const {
  return current_navigation_id_;
}

void AIChatDriver::SetNavigationId(int64_t navigation_id) {
  current_navigation_id_ = navigation_id;
}

bool AIChatDriver::IsSameDocumentNavigation() const {
  return is_same_document_navigation_;
}

void AIChatDriver::SetSameDocumentNavigation(bool same_document_navigation) {
  is_same_document_navigation_ = same_document_navigation;
}

std::vector<std::string> AIChatDriver::GetSuggestedQuestions(
    bool& can_generate,
    mojom::AutoGenerateQuestionsPref& auto_generate) {
  // Can we get suggested questions
  can_generate = !has_generated_questions_ && !article_text_.empty();
  // Are we allowed to auto-generate
  auto_generate = GetAutoGeneratePref();
  return suggested_questions_;
}

bool AIChatDriver::HasPageContent() {
  return !article_text_.empty();
}

void AIChatDriver::DisconnectPageContents() {
  CleanUp();

  should_page_content_be_disconnected_ = true;
}

void AIChatDriver::ClearConversationHistory() {
  chat_history_.clear();
  engine_->ClearAllQueries();

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

mojom::APIError AIChatDriver::GetCurrentAPIError() {
  return current_error_;
}

void AIChatDriver::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  DCHECK(HasUserOptedIn());
  DCHECK(is_conversation_active_);
  // Can't operate if we don't have an article text
  if (article_text_.empty()) {
    return;
  }
  // Don't perform the operation more than once
  if (suggested_questions_.size() > 1u) {
    return;
  }

  // Don't generate suggested questions if there's already on-going conversions
  if (!chat_history_.empty()) {
    return;
  }

  has_generated_questions_ = true;
  OnSuggestedQuestionsChanged();
  // Make API request for questions.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  auto navigation_id_for_query = current_navigation_id_;
  engine_->GenerateQuestionSuggestions(
      is_video_, article_text_,
      base::BindOnce(&AIChatDriver::OnSuggestedQuestionsResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(navigation_id_for_query)));
}

void AIChatDriver::OnSuggestedQuestionsResponse(
    int64_t navigation_id,
    std::vector<std::string> result) {
  // We might have navigated away whilst this async operation is in
  // progress, so check if we're the same navigation.
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  suggested_questions_.insert(suggested_questions_.end(), result.begin(),
                              result.end());
  // Notify observers
  OnSuggestedQuestionsChanged();
  DVLOG(2) << "Got questions:" << base::JoinString(suggested_questions_, "\n");
}

void AIChatDriver::MakeAPIRequestWithConversationHistoryUpdate(
    mojom::ConversationTurn turn) {
  if (!is_conversation_active_ || !HasUserOptedIn()) {
    // This function should not be presented in the UI if the user has not
    // opted-in yet.
    pending_request_ =
        std::make_unique<mojom::ConversationTurn>(std::move(turn));
    return;
  }

  DCHECK(turn.character_type == CharacterType::HUMAN);

  bool is_suggested_question = false;

  // If it's a suggested question, remove it
  auto found_question_iter =
      base::ranges::find(suggested_questions_, turn.text);
  if (found_question_iter != suggested_questions_.end()) {
    is_suggested_question = true;
    suggested_questions_.erase(found_question_iter);
    OnSuggestedQuestionsChanged();
  }

  // Directly modify Entry's text to remove engine-breaking substrings
  engine_->SanitizeInput(turn.text);

  // TODO(petemill): Tokenize the summary question so that we
  // don't have to do this weird substitution.
  std::string question_part;
  if (turn.text == "Summarize this video") {
    question_part =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO_BULLETS);
  } else {
    question_part = turn.text;
  }

  // Suggested questions were based on only the initial prompt (with content),
  // so no need to submit all conversation history when they are used.
  std::vector<mojom::ConversationTurn> history =
      is_suggested_question ? std::vector<mojom::ConversationTurn>()
                            : chat_history_;

  auto data_received_callback = base::BindRepeating(
      &AIChatDriver::OnEngineCompletionDataReceived,
      weak_ptr_factory_.GetWeakPtr(), current_navigation_id_);

  auto data_completed_callback =
      base::BindOnce(&AIChatDriver::OnEngineCompletionComplete,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id_);

  engine_->GenerateAssistantResponse(
      is_video_, article_text_, history, question_part,
      std::move(data_received_callback), std::move(data_completed_callback));

  // Add the human part to the conversation
  AddToConversationHistory(std::move(turn));

  is_request_in_progress_ = true;
}

void AIChatDriver::RetryAPIRequest() {
  SetAPIError(mojom::APIError::None);
  DCHECK(!chat_history_.empty());

  // We're using a reverse iterator here to find the latest human turn
  for (std::vector<ConversationTurn>::reverse_iterator rit =
           chat_history_.rbegin();
       rit != chat_history_.rend(); ++rit) {
    if (rit->character_type == CharacterType::HUMAN) {
      auto turn = *std::make_move_iterator(rit);
      auto human_turn_iter = rit.base() - 1;
      chat_history_.erase(human_turn_iter, chat_history_.end());
      MakeAPIRequestWithConversationHistoryUpdate(turn);
      break;
    }
  }
}

bool AIChatDriver::IsRequestInProgress() {
  return is_request_in_progress_;
}

void AIChatDriver::OnEngineCompletionDataReceived(int64_t navigation_id,
                                                  std::string result) {
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  UpdateOrCreateLastAssistantEntry(result);

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatDriver::OnEngineCompletionComplete(
    int64_t navigation_id,
    EngineConsumer::GenerationResult result) {
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  is_request_in_progress_ = false;

  if (result.has_value()) {
    // Handle success, which might mean do nothing much since all
    // data was passed in the streaming "received" callback.
    if (!result->empty()) {
      UpdateOrCreateLastAssistantEntry(*result);
    }
  } else {
    // handle failure
    SetAPIError(std::move(result.error()));
  }

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatDriver::OnSuggestedQuestionsChanged() {
  for (auto& obs : observers_) {
    obs.OnSuggestedQuestionsChanged(
        suggested_questions_, has_generated_questions_, GetAutoGeneratePref());
  }
}

void AIChatDriver::OnPageHasContentChanged() {
  for (auto& obs : observers_) {
    obs.OnPageHasContent();
  }
}

mojom::AutoGenerateQuestionsPref AIChatDriver::GetAutoGeneratePref() {
  mojom::AutoGenerateQuestionsPref pref =
      mojom::AutoGenerateQuestionsPref::Unset;

  const base::Value* auto_generate_value = pref_service_->GetUserPrefValue(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions);

  if (auto_generate_value) {
    pref = (auto_generate_value->GetBool()
                ? mojom::AutoGenerateQuestionsPref::Enabled
                : mojom::AutoGenerateQuestionsPref::Disabled);
  }

  return pref;
}

void AIChatDriver::SetAPIError(const mojom::APIError& error) {
  current_error_ = error;

  for (Observer& obs : observers_) {
    obs.OnAPIResponseError(current_error_);
  }
}

}  // namespace ai_chat
