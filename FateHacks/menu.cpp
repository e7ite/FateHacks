#include "menu.hpp"

#include <utility>

namespace {

using ::fate::CFontMetric;
using ::fate::CMaterial;
using ::fate::CText;
using ::fate::IDirect3DDevice8;
using ::fate::STLString;

// The scale an unhovered row is drawn at (a hovered row draws larger, see
// Menu::Render). Menu::IsItemHovered checks clicks against this same
// baseline, so a TextMeasurer backed by the real font measures what's
// actually on screen.
constexpr float kUnhoveredScale = 0.85f;

}  // namespace

void OverlayText::Render(IDirect3DDevice8* device, CMaterial* material,
                         CFontMetric* font, int x, int y, float scale) {
  STLString str(text_);
  if (ctext_ == nullptr) {
    ctext_ = CText::Create(device, material, font, &str, static_cast<float>(x),
                           static_cast<float>(y), scale, 0, 2, 1024, 768);
  } else {
    (ctext_.get()->*CText::Update)(device, &str, static_cast<float>(x),
                                   static_cast<float>(y), scale, 0, 2, 1024,
                                   768);
  }
  (ctext_.get()->*CText::Render)(device);
}

MenuItem Action(const char* label, std::function<void()> action) {
  return MenuItem{OverlayText{label}, std::move(action), {}};
}

MenuItem Submenu(const char* label, std::vector<MenuItem> items) {
  return MenuItem{OverlayText{label}, {}, std::move(items)};
}

Menu::Menu(const char* title, std::vector<MenuItem> items,
           TextMeasurer* measurer)
    : root_{OverlayText{title}, {}, std::move(items)}, measurer_(measurer) {}

int Menu::RowY(std::size_t index, std::size_t count) {
  constexpr int kCenterY = 768 / 2;
  constexpr int kRowSpacing = 58;
  int first_y = kCenterY - (static_cast<int>(count) - 1) * kRowSpacing / 2;
  return first_y + static_cast<int>(index) * kRowSpacing;
}

void Menu::Toggle() {
  open_ = !open_;
  nav_stack_ =
      {};  // always reopen at the root level (std::stack has no clear()).
}

void Menu::Back() {
  if (!open_) {
    return;
  }
  if (nav_stack_.empty()) {
    open_ = false;
  } else {
    nav_stack_.pop();
  }
}

const char* Menu::title() const { return CurrentParent().label.text(); }

std::size_t Menu::item_count() const { return CurrentParent().submenu.size(); }

const char* Menu::label_at(std::size_t index) const {
  return CurrentParent().submenu[index].label.text();
}

bool Menu::IsItemHovered(std::size_t index, int mouse_x, int mouse_y) const {
  constexpr int kMinHalfWidth = 80;
  constexpr int kHorizontalPadding = 18;
  constexpr int kTopPadding = 14;
  constexpr int kBottomPadding = 16;

  const std::vector<MenuItem>& items = CurrentParent().submenu;
  const char* label = items[index].label.text();
  int y = RowY(index, items.size());

  // The clickable box widens with the label's measured width so longer
  // entries stay easy to click, but never shrinks below a minimum width.
  int text_half_width =
      measurer_->MeasureWidth(label, kUnhoveredScale) / 2 + kHorizontalPadding;
  int half_width =
      text_half_width > kMinHalfWidth ? text_half_width : kMinHalfWidth;
  return mouse_x >= kCenterX - half_width && mouse_x <= kCenterX + half_width &&
         mouse_y >= y - kTopPadding && mouse_y <= y + kBottomPadding;
}

void Menu::Activate(int mouse_x, int mouse_y) {
  if (!open_) {
    return;
  }
  std::vector<MenuItem>& items = CurrentParent().submenu;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (!IsItemHovered(i, mouse_x, mouse_y)) {
      continue;
    }
    if (!items[i].submenu.empty()) {
      nav_stack_.push(&items[i]);  // descend into the submenu
    } else if (items[i].action) {
      items[i].action();
    }
    return;
  }
}

void Menu::Render(IDirect3DDevice8* device, CMaterial* material,
                  CFontMetric* font, int mouse_x, int mouse_y) {
  if (!open_) {
    return;
  }
  // The scale a hovered row is drawn at; brighter and larger than
  // kUnhoveredScale is how a hovered row reads as selected.
  constexpr float kHoveredScale = 1.0f;
  // The heading sits above the first item, drawn larger so it reads as a
  // title.
  constexpr float kTitleScale = 1.4f;
  constexpr int kTitleGap = 66;

  MenuItem& parent = CurrentParent();
  std::vector<MenuItem>& items = parent.submenu;
  parent.label.Render(device, material, font, kCenterX,
                      RowY(0, items.size()) - kTitleGap, kTitleScale);
  for (std::size_t i = 0; i < items.size(); ++i) {
    float scale =
        IsItemHovered(i, mouse_x, mouse_y) ? kHoveredScale : kUnhoveredScale;
    items[i].label.Render(device, material, font, kCenterX,
                          RowY(i, items.size()), scale);
  }
}

MenuItem& Menu::CurrentParent() {
  return nav_stack_.empty() ? root_ : *nav_stack_.top();
}

const MenuItem& Menu::CurrentParent() const {
  return nav_stack_.empty() ? root_ : *nav_stack_.top();
}

Menu BuildCheatMenu(CharacterActions* actions, TextMeasurer* measurer) {
  return Menu(
      "FateHacks",
      Items(
          Action("Add Gold", [actions] { actions->GiveGold(100); }),
          Submenu("More",
                  Items(Action("Add Gold x10",
                               [actions] {
                                 for (int i = 0; i < 10; ++i) {
                                   actions->GiveGold(100);
                                 }
                               }),
                        Submenu("Damage Multiplier",
                                Items(Action("x1 (Off)",
                                             [actions] {
                                               actions->SetDamageMultiplier(1);
                                             }),
                                      Action("x5",
                                             [actions] {
                                               actions->SetDamageMultiplier(5);
                                             }),
                                      Action("x10",
                                             [actions] {
                                               actions->SetDamageMultiplier(10);
                                             }),
                                      Action("x50",
                                             [actions] {
                                               actions->SetDamageMultiplier(50);
                                             })))))),
      measurer);
}
