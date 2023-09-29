/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/grit/generated_resources.h"

#undef IDS_PERMISSIONS_BUBBLE_PROMPT
#define IDS_PERMISSIONS_BUBBLE_PROMPT IDS_BRAVE_PERMISSIONS_BUBBLE_PROMPT
#include "src/chrome/browser/ui/views/file_system_access/file_system_access_restore_permission_bubble_view.cc"
#undef IDS_PERMISSIONS_BUBBLE_PROMPT
