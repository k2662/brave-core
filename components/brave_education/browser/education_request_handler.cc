/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_education/browser/education_request_handler.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_education {

namespace {

content::WebContents* WebContentsFromHostID(
    content::GlobalRenderFrameHostId frame_id) {
  auto* host = content::RenderFrameHost::FromID(frame_id);
  if (!host) {
    return nullptr;
  }

  return content::WebContents::FromRenderFrameHost(host);
}

}  // namespace

EducationRequestHandler::EducationRequestHandler(
    content::GlobalRenderFrameHostId frame_id)
    : frame_id_(frame_id) {}

EducationRequestHandler::~EducationRequestHandler() = default;

void EducationRequestHandler::ShowSettingsPage(const std::string& path,
                                               const std::string& highlight) {
  GURL settings_url("brave://settings/" + path);
  if (!settings_url.is_valid()) {
    return;
  }

  auto* web_contents = WebContentsFromHostID(frame_id_);
  if (!web_contents) {
    return;
  }

  web_contents->OpenURL({settings_url, content::Referrer(),
                        WindowOpenDisposition::NEW_FOREGROUND_TAB,
                        ui::PAGE_TRANSITION_LINK, false});
}

/*

=== Open settings with a promo/highlight ===

ShowPromoInPage::Params params;
params.target_url = chrome::GetSettingsUrl(chrome::kAppearanceSubPage);
params.bubble_anchor_id = kVerticalTabsSettingElementId;
params.bubble_arrow = user_education::HelpBubbleArrow::kBottomLeft;
params.bubble_text = l10n_util::GetStringUTF16(
    IDS_SETTINGS_VERTICAL_TABS_IPH_BUBBLE_TEXT);
params.close_button_alt_text_id =
    IDS_SETTINGS_VERTICAL_TABS_IPH_BUBBLE_CLOSE_BUTTON_ARIA_LABEL_TEXT;

if (auto* browser = chrome::FindLastActive()) {
  ShowPromoInPage::Start(browser, std::move(params));
}

=== Starting a tutorial ===

auto* service = UserEducationServiceFactory::GetForBrowserContext(
    browser_context());
if (!service) {
  return NoArguments();
}

auto* profile = Profile::FromBrowserContext(browser_context());
const ui::ElementContext context = chrome::FindBrowserWithProfile(profile)
    ->window()
    ->GetElementContext();
if (!context) {
  return NoArguments();
}

service->tutorial_service().StartTutorial("Brave VPN Tutorial", context);
return NoArguments();

*/

}  // namespace brave_education
