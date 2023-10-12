// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/md_select.css.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PrefsMixin} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {CrSettingsPrefs} from 'chrome://resources/cr_components/settings_prefs/prefs_types.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {getTemplate} from './brave_leo_assistant_page.html.js'
import {BraveLeoAssistantBrowserProxy, BraveLeoAssistantBrowserProxyImpl}
  from './brave_leo_assistant_browser_proxy.js'
import 'chrome://resources/brave/leo.bundle.js'

enum ModelEngineType {
  LLAMA_REMOTE,
  CLAUDE_REMOTE,
}

enum ModelCategory {
  CHAT
}

interface Models {
  key: string
  name: string
  display_name: string
  is_premium: boolean
  engine_type: ModelEngineType
  category: ModelCategory
}

const MODEL_NAMES = new Map([
  ['chat-default', 'Llama-2-13b'],
  ['chat-leo-expanded', 'Llama-2-70b'],
  ['chat-claude-instant', 'Claude Instant'],
])

const modelPrefPath = 'brave.ai_chat.default_model_key'

const BraveLeoAssistantPageBase =
  WebUiListenerMixin(I18nMixin(PrefsMixin(PolymerElement)))

/**
 * 'settings-brave-leo-assistant-page' is the settings page containing
 * brave's Leo Assistant features.
 */
class BraveLeoAssistantPageElement extends BraveLeoAssistantPageBase {
    static get is() {
        return 'settings-brave-leo-assistant-page'
    }

    static get template() {
        return getTemplate()
    }

    static get properties() {
      return {
        leoAssistantShowOnToolbarPref_: {
          type: Boolean,
          value: false,
          notify: true,
        }
      }
    }

    leoAssistantShowOnToolbarPref_: boolean
    defaultModelKeyPref_: string
    models_: Models[]

    browserProxy_: BraveLeoAssistantBrowserProxy =
      BraveLeoAssistantBrowserProxyImpl.getInstance()

    onResetAssistantData_() {
      const message =
        this.i18n('braveLeoAssistantResetAndClearDataConfirmationText')
      if(window.confirm(message)) {
        this.browserProxy_.resetLeoData()
      }
    }

    override ready () {
      super.ready()

      this.updateShowLeoAssistantIcon_()

      this.addWebUiListener('settings-brave-leo-assistant-changed',
      (isLeoVisible: boolean) => {
        this.leoAssistantShowOnToolbarPref_ = isLeoVisible
      })

      this.browserProxy_.getModels()
        .then((models: Models[]) => this.models_ = models)

      CrSettingsPrefs.initialized
        .then(() => {
          this.defaultModelKeyPref_ = this.getPref(modelPrefPath).value
        })
    }

    itemPref_(enabled: boolean) {
      return {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: enabled,
      }
    }

    toModelName_(modelKey: string) {
      return MODEL_NAMES.get(modelKey)
    }

    // TODO(nullhook): share these strings with AIChat webui
    getModelSubtitleLocale_(modelKey: string) {
      const key = modelKey.split('-').map(w => w.charAt(0).toUpperCase() + w.slice(1)).join('')
      return this.i18n(`braveLeo${key}Subtitle`)
    }

    onModelSelectionChange_(e: any) {
      this.setPrefValue(modelPrefPath, e.detail.value)
      this.defaultModelKeyPref_ = e.detail.value
    }

    private updateShowLeoAssistantIcon_() {
      this.browserProxy_.getLeoIconVisibility().then((result) => {
        this.leoAssistantShowOnToolbarPref_ = result
      })
    }

    onLeoAssistantShowOnToolbarChange_(e: any) {
      e.stopPropagation()
      this.browserProxy_.toggleLeoIcon()
    }
}

customElements.define(
  BraveLeoAssistantPageElement.is, BraveLeoAssistantPageElement)
