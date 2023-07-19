// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_education/renderer/education_page_enhancer.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_education/renderer/js_api_builder.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/blink.h"

namespace brave_education {

namespace {

static const char16_t kEnhancePageScript[] = uR"javascript({

  function getHighlight(elem) {
    const parent = elem.parentElement
    if (!parent) return ''
    if (/vertical tabs/i.test(parent.innerHTML)) {
      return 'vertical-tabs'
    }
    return ''
  }

  function linkifySettingsURLs() {
    let matches = []

    for (const elem of document.getElementsByTagName('code')) {
      const text = elem.innerText.trim()
      const path = text.replace(/^brave:\/\/settings\//, '')
      if (path !== text) {
        matches.push({
          element: elem,
          url: text,
          path: path,
          highlight: getHighlight(elem)
        })
      }
    }

    for (const { element, url, path, highlight } of matches) {
      const link = document.createElement('a')
      link.href = url
      link.onclick = (event) => {
        event.preventDefault()
        braveEducation.openSettingsPage(path, highlight)
      }
      element.parentNode.replaceChild(link, element)
      link.appendChild(element)
    }
  }

  linkifySettingsURLs()

  })javascript";

bool ShouldEnhancePage(content::RenderFrame* render_frame) {
  if (!render_frame || !render_frame->IsMainFrame()) {
    return false;
  }

  auto* web_frame = render_frame->GetWebFrame();
  DCHECK(web_frame);
  if (web_frame->IsProvisional()) {
    return false;
  }

  GURL origin = url::Origin(web_frame->GetSecurityOrigin()).GetURL();
  if (!origin.is_valid() || !origin.SchemeIs(url::kHttpsScheme) ||
      origin.host() != "brave.com") {
    return false;
  }

  GURL document_url = web_frame->GetDocument().Url();
  auto path = document_url.path_piece();
  return path == "/whats-new" || base::StartsWith(path, "/whats-new/");
}

}  // namespace

EducationPageEnhancer::EducationPageEnhancer(content::RenderFrame* render_frame,
                                             int32_t world_id)
    : RenderFrameObserver(render_frame),
      world_id_(world_id) {}

EducationPageEnhancer::~EducationPageEnhancer() = default;

void EducationPageEnhancer::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id == world_id_ && ShouldEnhancePage(render_frame()) &&
      !context.IsEmpty()) {
    InjectEducationRequestAPI(context);
  }
}

void EducationPageEnhancer::DidFinishLoad() {
  // TODO(zenparsing): Rather than calling `ShouldEnhancePage` again, perhaps
  // test once in `DidStartNavigation`?
  if (!ShouldEnhancePage(render_frame())) {
    return;
  }

  render_frame()->GetWebFrame()->ExecuteScriptInIsolatedWorld(
      world_id_,
      blink::WebScriptSource(
          blink::WebString::FromUTF16(kEnhancePageScript)),
      blink::BackForwardCacheAware::kAllow);
}

void EducationPageEnhancer::OnDestruct() {
  delete this;
}

mojo::Remote<mojom::EducationRequestHandler>&
EducationPageEnhancer::GetRequestHandler() {
  if (!request_handler_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        request_handler_.BindNewPipeAndPassReceiver());
  }
  CHECK(request_handler_.is_bound());
  return request_handler_;
}

void EducationPageEnhancer::InjectEducationRequestAPI(
    v8::Local<v8::Context> context) {
  // TODO(zenparsing): Do we actually need to check whether the object has
  // already been installed?
  JSAPIBuilder::Create(blink::MainThreadIsolate(), context)
      .SetMethod(
          "openSettingsPage",
          base::BindRepeating(&EducationPageEnhancer::ShowSettingsPageCallback,
                              weak_factory_.GetWeakPtr()))
      .SetAsObjectProperty(context->Global(), "braveEducation");
}

void EducationPageEnhancer::ShowSettingsPageCallback(
    const std::string& path,
    const std::string& highlight) {
  GetRequestHandler()->ShowSettingsPage(path, highlight);
}

}  // namespace brave_education
