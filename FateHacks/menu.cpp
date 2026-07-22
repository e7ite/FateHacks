#include "menu.hpp"

#include <utility>

namespace {

using ::fate::CFontMetric;
using ::fate::CMaterial;
using ::fate::CText;
using ::fate::IDirect3DDevice8;
using ::fate::STLString;

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

Menu::Menu(const char* title, std::vector<MenuItem> items)
    : root_{OverlayText{title}, {}, std::move(items)} {}

void Menu::Toggle() {
  open_ = !open_;
  nav_stack_ =
      {};  // always reopen at the root level (std::stack has no clear()).
}

const char* Menu::title() const { return CurrentParent().label.text(); }

MenuItem& Menu::CurrentParent() {
  return nav_stack_.empty() ? root_ : *nav_stack_.top();
}

const MenuItem& Menu::CurrentParent() const {
  return nav_stack_.empty() ? root_ : *nav_stack_.top();
}

Menu BuildCheatMenu(CharacterActions* actions) {
  return Menu("FateHacks",
              Items(Action("Add Gold", [actions] { actions->GiveGold(100); }),
                    Submenu("More", Items(Action("Add Gold x10", [actions] {
                              for (int i = 0; i < 10; ++i) {
                                actions->GiveGold(100);
                              }
                            })))));
}
