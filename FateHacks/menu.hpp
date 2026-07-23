#ifndef FATEHACKS_MENU_HPP_
#define FATEHACKS_MENU_HPP_

#include <cstddef>
#include <functional>
#include <memory>
#include <stack>
#include <vector>

#include "render.hpp"

// The player-character cheats a menu item can trigger. Injected so an item can
// be activated in a test without the game's real character object present.
class CharacterActions {
 public:
  virtual ~CharacterActions() = default;
  virtual void GiveGold(int amount) = 0;
  virtual void SetDamageMultiplier(int multiplier) = 0;
  virtual void AdjustWeaponDamageDealtBonus(int delta) = 0;
};

// Measures the on-screen width of menu label text at the given scale, in
// pixels. Injected because real measurement depends on the game's loaded
// font; tests substitute a fake with known widths.
class TextMeasurer {
 public:
  virtual ~TextMeasurer() = default;
  virtual int MeasureWidth(const char* text, float scale) const = 0;
};

// A single line of menu text that owns its CText: created lazily on first draw,
// then restyled in place each frame via CText::Update -- the way the game edits
// its own text -- so the render loop never reallocates GPU objects. Building
// one (with no CText yet) never touches the game; only Render does.
class OverlayText {
 public:
  explicit OverlayText(const char* text) : text_(text) {}

  const char* text() const { return text_; }

  void Render(fate::IDirect3DDevice8* device, fate::CMaterial* material,
              fate::CFontMetric* font, int x, int y, float scale);

 private:
  const char* text_;
  // A function-pointer deleter isn't default-constructible, so both members are
  // supplied up front (null pointer, null deleter) until Render hands us a live
  // CText.
  std::unique_ptr<fate::CText, void (*)(fate::CText*)> ctext_{nullptr, nullptr};
};

// One entry in the menu: a label plus either an `action` (a leaf) or a nested
// `submenu` (a node, when `submenu` is non-empty).
struct MenuItem {
  OverlayText label;
  std::function<void()> action;
  std::vector<MenuItem> submenu;
};

// Builders for menu trees. MenuItem is move-only, so a list is built by moving
// items in, not with a braced initializer list.
MenuItem Action(const char* label, std::function<void()> action);
MenuItem Submenu(const char* label, std::vector<MenuItem> items);

template <typename... Ts>
std::vector<MenuItem> Items(Ts&&... items) {
  std::vector<MenuItem> list;
  list.reserve(sizeof...(items));
  // A comma fold expression: push_back is called once per pack argument, in
  // order, each item perfect-forwarded so it's moved (not copied) into the
  // vector.
  (list.push_back(std::forward<Ts>(items)), ...);
  return list;
}

// A navigable, centered list of menu items. Left-click activation runs a leaf
// item's action or descends into a submenu; Back steps up one level, or
// closes the menu if already at the main menu.
class Menu {
 public:
  // `title` is the heading shown at the main menu (before descending into a
  // submenu). `measurer` is used to check clicks against each row's
  // rendered width; it must outlive the Menu.
  Menu(const char* title, std::vector<MenuItem> items, TextMeasurer* measurer);

  // Screen-center X in the game's virtual 1024x768 space; rows center here.
  // Public so tests can build the coordinates of a specific row to click.
  static constexpr int kCenterX = 1024 / 2;

  // The Y of row `index` when `count` rows are shown, with the block centered
  // vertically. Public for the same reason as kCenterX.
  static int RowY(std::size_t index, std::size_t count);

  // Opens the menu if closed, closes it if open; always reopens at the main
  // menu.
  void Toggle();

  // Steps out of the current submenu, or closes the menu if already at the
  // main menu.
  void Back();

  bool is_open() const { return open_; }

  // The heading for the level currently shown: the menu's title at the main
  // menu, or (once submenu descent exists) the label of the item last
  // descended into.
  const char* title() const;

  // Number of items on the level currently shown (the main menu or a
  // submenu).
  std::size_t item_count() const;

  // The label of the item at `index` on the level currently shown; pair with
  // item_count() to draw the visible rows.
  const char* label_at(std::size_t index) const;

  // True when (mouse_x, mouse_y) is over item `index` on the level currently
  // shown. Query-only -- unlike Activate, it never runs the item's action or
  // descends into its submenu; rendering uses it to highlight the hovered row.
  bool IsItemHovered(std::size_t index, int mouse_x, int mouse_y) const;

  // Activates the item under the mouse: runs a leaf's action or descends into a
  // submenu. Does nothing if the menu is closed or nothing is hovered.
  void Activate(int mouse_x, int mouse_y);

  // Draws the title and every visible row, brightened when hovered. The only
  // member that touches the game: everything else above is plain data and
  // logic, testable without a device.
  void Render(fate::IDirect3DDevice8* device, fate::CMaterial* material,
              fate::CFontMetric* font, int mouse_x, int mouse_y);

 private:
  // The item whose label is the heading and whose submenu is the item list
  // currently shown: `root_` at the root, or the item last descended into
  // inside a submenu. Always exists -- `root_` stands in as the parent of the
  // root items -- so this never needs to represent "no current parent".
  MenuItem& CurrentParent();
  const MenuItem& CurrentParent() const;

  MenuItem root_;
  TextMeasurer* measurer_;
  bool open_ = false;
  // Items descended into, root-to-leaf; always empty until something can
  // actually descend into a submenu. The tree's shape never changes after
  // construction (root_'s nested vectors are built once in BuildCheatMenu and
  // never resized), so pointers into it stay valid for the Menu's lifetime --
  // as long as the Menu itself is never moved or reassigned after something
  // pushes onto this stack.
  std::stack<MenuItem*> nav_stack_;
};

// Builds the FateHacks cheat menu, wiring every item's action to `actions` and
// checking clicks against `measurer`.
Menu BuildCheatMenu(CharacterActions* actions, TextMeasurer* measurer);

#endif  // FATEHACKS_MENU_HPP_
