// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_

#include "chrome/browser/ui/views/frame/browser_view_layout.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class BraveBrowserViewLayout : public BrowserViewLayout {
 public:
  using BrowserViewLayout::BrowserViewLayout;
  ~BraveBrowserViewLayout() override;

  void set_contents_background(views::View* contents_background) {
    contents_background_ = contents_background;
  }

  void set_vertical_tab_strip_host(views::View* vertical_tab_strip_host) {
    vertical_tab_strip_host_ = vertical_tab_strip_host;
  }

  void set_reader_mode_toolbar(views::View* reader_mode_toolbar) {
    reader_mode_toolbar_ = reader_mode_toolbar;
  }

  void set_sidebar_container(views::View* sidebar_container) {
    sidebar_container_ = sidebar_container;
  }

  void set_sidebar_on_left(bool sidebar_on_left) {
    sidebar_on_left_ = sidebar_on_left;
  }

  void set_sidebar_separator(views::View* sidebar_separator) {
    sidebar_separator_ = sidebar_separator;
  }

  // Returns the ideal sidebar width, given the current available width. Used
  // for determining the target width in sidebar width animations.
  int GetIdealSideBarWidth() const;
  int GetIdealSideBarWidth(int available_width) const;

  // BrowserViewLayout:
  void Layout(views::View* host) override;
  void LayoutSidePanelView(views::View* side_panel,
                           gfx::Rect& contents_container_bounds) override;
  int LayoutTabStripRegion(int top) override;
  int LayoutBookmarkAndInfoBars(int top, int browser_view_y) override;
  int LayoutInfoBar(int top) override;
  void LayoutContentsContainerView(int top, int bottom) override;

 private:
  class ScopedVerticalTabsLayoutOffset;

  void LayoutSideBar(gfx::Rect& contents_bounds);
  void LayoutReaderModeToolbar(gfx::Rect& contents_bounds);
  void LayoutVerticalTabs();

  // Returns the padding to be applied to the main contents area, depending upon
  // which other views are adjacent.
  gfx::Insets GetContentsPadding() const;

  // Returns a value indicating whether the bookmark bar is currently visible,
  // but is not always shown. This can happen when the bookmarks bar is visible
  // on the NTP. In that case we will lay out the vertical tab strip next to the
  // bookmarks bar so that the tab strip doesn't move when changing the active
  // tab.
  bool IsBookmarkBarTemporarilyVisible() const;

  // In vertical tabs mode, modifies upstream's layout logic by temporarily
  // overwriting internal layout position state. Do not call this method
  // directly; use `ScopedVerticalTabsLayoutOffset` instead.
  absl::optional<gfx::Rect> MaybeOffsetLayoutForVerticalTabs(int top);

  // Resets the current layout offset, if active.
  void ResetLayoutOffset(const gfx::Rect& layout_rect);

  // Returns the insets that should be applied to the vertical tabstrip
  // container in order to account for OS-specified frame borders.
  gfx::Insets GetVerticalTabsInsetsForFrameBorder() const;

  raw_ptr<views::View> vertical_tab_strip_host_ = nullptr;
  raw_ptr<views::View> reader_mode_toolbar_ = nullptr;
  raw_ptr<views::View> sidebar_container_ = nullptr;
  raw_ptr<views::View> sidebar_separator_ = nullptr;
  raw_ptr<views::View> contents_background_ = nullptr;
  absl::optional<int> vertical_tabs_top_;
  bool sidebar_on_left_ = false;
  bool has_layout_offset_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
