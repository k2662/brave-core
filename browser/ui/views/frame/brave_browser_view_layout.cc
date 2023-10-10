// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"

#include <algorithm>
#include <limits>

#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "ui/views/border.h"

namespace {

constexpr int kSidebarSeparatorWidth = 1;

constexpr int kSidebarSeparatorMargin = 4;

#if BUILDFLAG(IS_MAC)
constexpr int kVerticalTabsFrameBorderMargin = 1;
#else
constexpr int kVerticalTabsFrameBorderMargin = 0;
#endif  // BUILDFLAG(IS_MAC)

}  // namespace

class BraveBrowserViewLayout::ScopedVerticalTabsLayoutOffset {
 public:
  explicit ScopedVerticalTabsLayoutOffset(BraveBrowserViewLayout* layout)
      : layout_(layout) {
    DCHECK(layout);
  }

  ~ScopedVerticalTabsLayoutOffset() {
    if (saved_layout_rect_) {
      layout_->ResetLayoutOffset(*saved_layout_rect_);
    }
  }

  void MaybeApply(int top) {
    saved_layout_rect_ = layout_->MaybeOffsetLayoutForVerticalTabs(top);
  }

 private:
  raw_ptr<BraveBrowserViewLayout> layout_ = nullptr;
  absl::optional<gfx::Rect> saved_layout_rect_;
};

BraveBrowserViewLayout::~BraveBrowserViewLayout() = default;

int BraveBrowserViewLayout::GetIdealSideBarWidth() const {
  if (!sidebar_container_) {
    return 0;
  }

  return GetIdealSideBarWidth(contents_container_->width() +
                              GetContentsPadding().width() +
                              sidebar_container_->width());
}

int BraveBrowserViewLayout::GetIdealSideBarWidth(int available_width) const {
  if (!sidebar_container_) {
    return 0;
  }

  int sidebar_width = sidebar_container_->GetPreferredSize().width();

  // The sidebar can take up the entire space for fullscreen.
  if (sidebar_width == std::numeric_limits<int>::max()) {
    return available_width;
  }

  // The sidebar should take up no more than 80% of the content area.
  return std::min<int>(available_width * 0.8, sidebar_width);
}

void BraveBrowserViewLayout::Layout(views::View* host) {
  vertical_tabs_top_ = absl::nullopt;
  BrowserViewLayout::Layout(host);
  LayoutVerticalTabs();
}

void BraveBrowserViewLayout::LayoutSidePanelView(
    views::View* side_panel,
    gfx::Rect& contents_container_bounds) {
  if (contents_background_) {
    contents_background_->SetBoundsRect(contents_container_bounds);
  }

  LayoutSideBar(contents_container_bounds);
  LayoutReaderModeToolbar(contents_container_bounds);

  contents_container_bounds.Inset(GetContentsPadding());
}

int BraveBrowserViewLayout::LayoutTabStripRegion(int top) {
  if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser())) {
    // In case we're using vertical tabstrip, we can decide the position
    // after we finish laying out views in top container.
    return top;
  }

  return BrowserViewLayout::LayoutTabStripRegion(top);
}

int BraveBrowserViewLayout::LayoutBookmarkAndInfoBars(int top,
                                                      int browser_view_y) {
  ScopedVerticalTabsLayoutOffset offset(this);

  // If the bookmark bar is temporarily visible (i.e. only visible on some
  // pages) and we are in vertical tab mode, then it should be positioned
  // adjacent to the vertical tabstrip. Offset the position of the bookmark bar
  // and subsequent views accordingly.
  if (IsBookmarkBarTemporarilyVisible()) {
    offset.MaybeApply(top);
  }

  return BrowserViewLayout::LayoutBookmarkAndInfoBars(top, browser_view_y);
}

int BraveBrowserViewLayout::LayoutInfoBar(int top) {
  ScopedVerticalTabsLayoutOffset offset(this);
  offset.MaybeApply(top);
  return BrowserViewLayout::LayoutInfoBar(top);
}

void BraveBrowserViewLayout::LayoutContentsContainerView(int top, int bottom) {
  ScopedVerticalTabsLayoutOffset offset(this);
  offset.MaybeApply(top);
  return BrowserViewLayout::LayoutContentsContainerView(top, bottom);
}

void BraveBrowserViewLayout::LayoutSideBar(gfx::Rect& contents_bounds) {
  if (!sidebar_container_) {
    return;
  }

  gfx::Rect sidebar_bounds = contents_bounds;
  sidebar_bounds.set_width(GetIdealSideBarWidth(contents_bounds.width()));

  contents_bounds.set_width(contents_bounds.width() - sidebar_bounds.width());

#if BUILDFLAG(IS_MAC)
  // On Mac, setting an empty rect for the contents web view could cause a crash
  // in `StatusBubbleViews`. As the `StatusBubbleViews` width is one third of
  // the base view, set 3 here so that `StatusBubbleViews` can have a width of
  // at least 1.
  if (contents_bounds.width() <= 0) {
    contents_bounds.set_width(3);
  }
#endif

  gfx::Rect separator_bounds;

  if (sidebar_on_left_) {
    contents_bounds.set_x(contents_bounds.x() + sidebar_bounds.width());

    // When vertical tabs and the sidebar are adjacent, add a separator between
    // them.
    if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) &&
        sidebar_separator_ && !sidebar_bounds.IsEmpty()) {
      separator_bounds = sidebar_bounds;
      separator_bounds.set_width(kSidebarSeparatorWidth);
      separator_bounds.Inset(gfx::Insets::VH(kSidebarSeparatorMargin, 0));

      // Move sidebar and content over to make room for the separator.
      sidebar_bounds.set_x(sidebar_bounds.x() + kSidebarSeparatorWidth);
      contents_bounds.Inset(gfx::Insets::TLBR(0, kSidebarSeparatorWidth, 0, 0));
    }

  } else {
    sidebar_bounds.set_x(contents_bounds.right());
  }

  sidebar_container_->SetBoundsRect(
      browser_view_->GetMirroredRect(sidebar_bounds));

  if (sidebar_separator_) {
    if (separator_bounds.IsEmpty()) {
      sidebar_separator_->SetVisible(false);
    } else {
      sidebar_separator_->SetBoundsRect(
          browser_view_->GetMirroredRect(separator_bounds));
      sidebar_separator_->SetVisible(true);
    }
  }
}

void BraveBrowserViewLayout::LayoutReaderModeToolbar(
    gfx::Rect& contents_bounds) {
  if (!reader_mode_toolbar_ || !reader_mode_toolbar_->GetVisible()) {
    return;
  }

  gfx::Rect bounds = contents_bounds;
  bounds.set_height(reader_mode_toolbar_->GetPreferredSize().height());
  reader_mode_toolbar_->SetBoundsRect(bounds);

  contents_bounds.Inset(gfx::Insets::TLBR(bounds.height(), 0, 0, 0));
}

void BraveBrowserViewLayout::LayoutVerticalTabs() {
  if (!vertical_tab_strip_host_) {
    return;
  }

  if (!tabs::utils::ShouldShowVerticalTabs(browser_view_->browser())) {
    vertical_tab_strip_host_->SetBorder(nullptr);
    vertical_tab_strip_host_->SetBoundsRect({});
    return;
  }

  gfx::Rect bounds = vertical_layout_rect_;
  CHECK(vertical_tabs_top_.has_value());
  bounds.SetVerticalBounds(*vertical_tabs_top_,
                           browser_view_->bounds().bottom());

  gfx::Insets insets = GetVerticalTabsInsetsForFrameBorder();
  bounds.set_width(vertical_tab_strip_host_->GetPreferredSize().width() +
                   insets.width());

  vertical_tab_strip_host_->SetBorder(
      insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));
  vertical_tab_strip_host_->SetBoundsRect(bounds);
}

gfx::Insets BraveBrowserViewLayout::GetContentsPadding() const {
  if (!base::FeatureList::IsEnabled(features::kBravePaddedWebContent)) {
    return {};
  }

  // Ideally we'd only apply padding where needed and let the shadow render on
  // top of other views. However, other views also render to layers and those
  // layers happen to be above the content layer.
  return gfx::Insets(BraveContentsViewUtil::kMargin);
}

bool BraveBrowserViewLayout::IsBookmarkBarTemporarilyVisible() const {
  auto* prefs = browser_view_->browser()->profile()->GetPrefs();
  return bookmark_bar_ &&
         !prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar) &&
         delegate_->IsBookmarkBarVisible();
}

absl::optional<gfx::Rect>
BraveBrowserViewLayout::MaybeOffsetLayoutForVerticalTabs(int top) {
  if (has_layout_offset_) {
    return absl::nullopt;
  }

  if (!tabs::utils::ShouldShowVerticalTabs(browser_view_->browser())) {
    return absl::nullopt;
  }

  // The first time we apply the offset, record the current layout `top`. This
  // will become the `y` position of the vertical tabstrip.
  if (!vertical_tabs_top_) {
    vertical_tabs_top_ = top;
  }

  gfx::Insets insets;
  CHECK(vertical_tab_strip_host_);
  insets.set_left(vertical_tab_strip_host_->GetPreferredSize().width());
  insets += GetVerticalTabsInsetsForFrameBorder();

  gfx::Rect current_layout_rect = vertical_layout_rect_;
  vertical_layout_rect_.Inset(insets);
  has_layout_offset_ = true;
  return current_layout_rect;
}

void BraveBrowserViewLayout::ResetLayoutOffset(const gfx::Rect& layout_rect) {
  if (has_layout_offset_) {
    vertical_layout_rect_ = layout_rect;
    has_layout_offset_ = false;
  }
}

gfx::Insets BraveBrowserViewLayout::GetVerticalTabsInsetsForFrameBorder()
    const {
  gfx::Insets insets;
  if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) ||
      !browser_view_->IsFullscreen()) {
    insets.set_left(kVerticalTabsFrameBorderMargin);
  }
  return insets;
}
