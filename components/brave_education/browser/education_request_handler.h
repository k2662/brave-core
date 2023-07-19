/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_EDUCATION_EDUCATION_REQUEST_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_EDUCATION_EDUCATION_REQUEST_HANDLER_H_

#include "brave/components/brave_education/common/mojom/brave_education.mojom.h"
#include "content/public/browser/global_routing_id.h"

namespace brave_education {

class EducationRequestHandler : public mojom::EducationRequestHandler {
 public:
  explicit EducationRequestHandler(content::GlobalRenderFrameHostId frame_id);
  ~EducationRequestHandler() override;

  // mojom::EducationRequestHandler:
  void ShowSettingsPage(const std::string& path, const std::string& highlight)
      override;

 private:
  content::GlobalRenderFrameHostId frame_id_;
};

}  // namespace brave_education

#endif  // BRAVE_COMPONENTS_BRAVE_EDUCATION_EDUCATION_REQUEST_HANDLER_H_
