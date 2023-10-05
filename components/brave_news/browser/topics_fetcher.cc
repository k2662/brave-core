// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/topics_fetcher.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/urls.h"

namespace brave_news {

namespace {

std::vector<Topic> ParseTopics(const std::string& topics_json,
                               const std::string& topic_articles_json) {
  std::vector<Topic> result;
  return result;
}

}  // namespace

TopicArticle::TopicArticle() = default;
TopicArticle::~TopicArticle() = default;

Topic::Topic() = default;
Topic::~Topic() = default;

TopicsFetcher::FetchState::FetchState(std::string locale,
                                      TopicsCallback callback)
    : locale(locale), callback(std::move(callback)) {}
TopicsFetcher::FetchState::~FetchState() = default;
TopicsFetcher::FetchState::FetchState(FetchState&&) = default;

TopicsFetcher::TopicsFetcher(
    api_request_helper::APIRequestHelper& api_request_helper)
    : api_request_helper_(api_request_helper) {}

TopicsFetcher::~TopicsFetcher() = default;

void TopicsFetcher::GetTopics(const std::string& locale,
                              TopicsCallback callback) {
  FetchTopics(FetchState(locale, std::move(callback)));
}

void TopicsFetcher::FetchTopics(FetchState state) {
  GURL url(base::StrCat({brave_news::GetHostname(), kTopicsEndpoint, ".",
                         state.locale, ".json"}));
  api_request_helper_->Request(
      "GET", url, "", "",
      base::BindOnce(&TopicsFetcher::OnFetchedTopics, base::Unretained(this),
                     std::move(state)));
}

void TopicsFetcher::OnFetchedTopics(
    FetchState state,
    api_request_helper::APIRequestResult result) {
  state.topics_json = std::move(result.body());
  FetchTopicArticles(std::move(state));
}

void TopicsFetcher::FetchTopicArticles(FetchState state) {
  GURL url(base::StrCat({brave_news::GetHostname(), kTopicArticlesEndpoint, ".",
                         state.locale, ".json"}));
  api_request_helper_->Request(
      "GET", url, "", "",
      base::BindOnce(&TopicsFetcher::OnFetchedTopicArticles,
                     base::Unretained(this), std::move(state)));
}

void TopicsFetcher::OnFetchedTopicArticles(
    FetchState state,
    api_request_helper::APIRequestResult result) {
  state.topic_articles_json = std::move(result.body());

  auto topics = ParseTopics(state.topics_json, state.topic_articles_json);
  std::move(state.callback).Run(std::move(topics));
}

}  // namespace brave_news
