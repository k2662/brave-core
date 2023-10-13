/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync.settings;

import android.os.Bundle;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.fragment.app.FragmentManager;
import androidx.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.password_manager.settings.ReauthenticationManager;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.ui.widget.Toast;

import java.util.Timer;
import java.util.TimerTask;

/**
 * See org.brave.bytecode.BraveManageSyncSettingsClassAdapter
 */
public class BraveManageSyncSettings extends ManageSyncSettings {
    private static final String TAG = "BMSS";

    private static final String PREF_ADVANCED_CATEGORY = "advanced_category";

    private Preference mGoogleActivityControls;
    private Preference mSyncEncryption;

    private ChromeSwitchPreference mPrefSyncPasswords;
    private ChromeSwitchPreference mSyncEverything;

    private BravePasswordAccessReauthenticationHelper mReauthenticationHelper;

    private Timer mPasswordsSummaryUpdater;

    @VisibleForTesting
    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        Preference reviewSyncData = findPreference(PREF_SYNC_REVIEW_DATA);
        assert reviewSyncData != null : "Something has changed in the upstream!";
        if (reviewSyncData != null) {
            getPreferenceScreen().removePreference(reviewSyncData);
        }

        Preference turnOffSync = findPreference(PREF_TURN_OFF_SYNC);
        assert turnOffSync != null : "Something has changed in the upstream!";
        if (turnOffSync != null) {
            getPreferenceScreen().removePreference(turnOffSync);
        }

        Preference syncReadingList = findPreference(PREF_SYNC_READING_LIST);
        assert syncReadingList != null : "Something has changed in the upstream!";
        if (syncReadingList != null) {
            syncReadingList.setVisible(false);
        }

        Preference syncAutofill = findPreference(PREF_SYNC_AUTOFILL);
        assert syncAutofill != null : "Something has changed in the upstream!";
        if (syncAutofill != null) {
            syncAutofill.setTitle(R.string.brave_sync_autofill);
        }

        assert mSyncEverything != null : "Something has changed in the upstream!";

        getPreferenceScreen().removePreference(mGoogleActivityControls);
        getPreferenceScreen().removePreference(mSyncEncryption);

        findPreference(PREF_ADVANCED_CATEGORY).setVisible(false);

        Preference syncPaymentsIntegration = findPreference(PREF_SYNC_PAYMENTS_INTEGRATION);
        assert syncPaymentsIntegration != null : "Something has changed in the upstream!";
        if (syncPaymentsIntegration != null) {
            syncPaymentsIntegration.setVisible(false);
        }

        mPrefSyncPasswords = findPreference(PREF_SYNC_PASSWORDS);
        assert mPrefSyncPasswords != null : "Something has changed in the upstream!";

        overrideWithAuthConfirmationSyncPasswords();
        overrideWithAuthConfirmationSyncEverything();
    }

    private void showScreenLockToast() {
        Toast.makeText(ContextUtils.getApplicationContext(),
                     R.string.password_sync_type_set_screen_lock, Toast.LENGTH_LONG)
                .show();
    }

    private void overrideWithAuthConfirmationSyncPasswords() {
        overrideWithAuthConfirmation(mPrefSyncPasswords);
    }

    private void overrideWithAuthConfirmationSyncEverything() {
        overrideWithAuthConfirmation(mSyncEverything);
    }

    private void overrideWithAuthConfirmation(ChromeSwitchPreference control) {
        Preference.OnPreferenceChangeListener origSyncListner =
                control.getOnPreferenceChangeListener();

        control.setOnPreferenceChangeListener((Preference preference, Object newValue) -> {
            assert newValue instanceof Boolean;
            if ((Boolean) newValue) {
                if (!ReauthenticationManager.isScreenLockSetUp(
                            ContextUtils.getApplicationContext())) {
                    showScreenLockToast();
                } else {
                    try {
                        FragmentManager fragmentManager = this.getParentFragmentManager();

                        if (mReauthenticationHelper == null) {
                            mReauthenticationHelper = new BravePasswordAccessReauthenticationHelper(
                                    ContextUtils.getApplicationContext(), fragmentManager);
                        }

                        mReauthenticationHelper.reauthenticateWithDescription(
                                R.string.enabling_password_sync_auth_message, success -> {
                                    if (success) {
                                        origSyncListner.onPreferenceChange(preference, true);
                                        control.setChecked(true);

                                        // Authentication will be valid for
                                        // ReauthenticationManager.
                                        // VALID_REAUTHENTICATION_TIME_INTERVAL_MILLIS,
                                        // So schedule re-check operation.
                                        scheduleCheckForStillValidAuth();
                                    }
                                });
                    } catch (java.lang.IllegalStateException ex) {
                        Log.e(TAG, "BraveManageSyncSettings.OnPreferenceChange ex=", ex);
                    }
                }
                return false;
            } else {
                return origSyncListner.onPreferenceChange(preference, newValue);
            }
        });
    }

    // See CredentialEditCoordinator.onResumeFragment
    public void onResumeFragment() {
        if (mReauthenticationHelper != null) {
            mReauthenticationHelper.onReauthenticationMaybeHappened();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        onResumeFragment();

        // May happen that person will switch to system settings and enable biometrics/passcode
        // authentication. In this case we must change summary
        updateSyncPasswordsSummary();
    }

    private void updateSyncPasswordsSummary() {
        if (ReauthenticationManager.isScreenLockSetUp(ContextUtils.getApplicationContext())) {
            if (ReauthenticationManager.authenticationStillValid(
                        ReauthenticationManager.ReauthScope.ONE_AT_A_TIME)) {
                mPrefSyncPasswords.setSummaryOff("");
            } else {
                setRedPasswordsSummaryOff(R.string.sync_password_require_auth_summary);
            }
        } else {
            setRedPasswordsSummaryOff(R.string.device_require_auth_to_sync_passwords_summary);
        }
    }

    private void setRedPasswordsSummaryOff(int stringId) {
        Spannable summary = new SpannableString(
                ContextUtils.getApplicationContext().getResources().getString(stringId));
        summary.setSpan(
                new ForegroundColorSpan(android.graphics.Color.RED), 0, summary.length(), 0);
        mPrefSyncPasswords.setSummaryOff(summary);
    }

    private void scheduleCheckForStillValidAuth() {
        if (mPasswordsSummaryUpdater == null) {
            mPasswordsSummaryUpdater = new Timer();
        } else {
            mPasswordsSummaryUpdater.cancel();
        }
        mPasswordsSummaryUpdater.schedule(
                new TimerTask() {
                    @Override
                    public void run() {
                        getActivity().runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                updateSyncPasswordsSummary();
                            }
                        });
                    }
                },
                // Will recheck authenticationStillValid after the time it must be discarded
                ReauthenticationManager.VALID_REAUTHENTICATION_TIME_INTERVAL_MILLIS + 1000);
    }
}
