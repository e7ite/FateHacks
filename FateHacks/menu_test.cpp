// Tests the cheat menu: opening and closing it, navigating it, and the
// character cheats each item triggers.

#include "menu.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::MockFunction;

// A minimal two-item menu for exercising navigation without any character
// cheat.
Menu MakeMenu() {
  return Menu("Title", Items(Action("One", [] {}), Action("Two", [] {})));
}

// Same as MakeMenu(), but the first item's action calls `func`, for tests
// that need to see whether activation actually ran an item's action.
Menu MakeMenu(MockFunction<void()>* func) {
  return Menu("Title", Items(Action("One", func->AsStdFunction()),
                             Action("Two", [] {})));
}

// Records the player-character cheats the menu triggers, standing in for the
// real game character so tests can assert on what a menu item does.
class MockCharacterActions final : public CharacterActions {
 public:
  MOCK_METHOD(void, GiveGold, (int amount), (override));
};

TEST(MenuTest, StartsClosed) {
  Menu menu = MakeMenu();

  EXPECT_FALSE(menu.is_open());
}

TEST(MenuTest, ToggleOpensClosedMenu) {
  Menu menu = MakeMenu();

  menu.Toggle();

  EXPECT_TRUE(menu.is_open());
}

TEST(MenuTest, ToggleAgainClosesOpenMenu) {
  Menu menu = MakeMenu();
  menu.Toggle();

  menu.Toggle();

  EXPECT_FALSE(menu.is_open());
}

TEST(MenuTest, TitleAtRootIsTheMenuTitle) {
  Menu menu = MakeMenu();
  menu.Toggle();

  EXPECT_STREQ(menu.title(), "Title");
}

TEST(MenuTest, BackAtRootClosesMenu) {
  Menu menu = MakeMenu();
  menu.Toggle();

  menu.Back();

  EXPECT_FALSE(menu.is_open());
}

TEST(MenuTest, RootShowsTwoItems) {
  Menu menu = MakeMenu();
  menu.Toggle();

  EXPECT_EQ(menu.item_count(), 2u);
}

TEST(MenuTest, ItemHoveredAtItsRowIsTrue) {
  Menu menu = MakeMenu();
  menu.Toggle();

  EXPECT_TRUE(menu.IsItemHovered(
      /*index=*/0, Menu::kCenterX, Menu::RowY(/*index=*/0, menu.item_count())));
}

TEST(MenuTest, ItemHoveredAwayFromItsRowIsFalse) {
  Menu menu = MakeMenu();
  menu.Toggle();

  EXPECT_FALSE(menu.IsItemHovered(/*index=*/0, /*mouse_x=*/0, /*mouse_y=*/0));
}

TEST(MenuTest, ItemHoveredDoesNotActivate) {
  MockFunction<void()> func;
  Menu menu = MakeMenu(&func);
  menu.Toggle();

  EXPECT_CALL(func, Call).Times(0);
  EXPECT_TRUE(menu.IsItemHovered(
      /*index=*/0, Menu::kCenterX, Menu::RowY(/*index=*/0, menu.item_count())));
}

TEST(MenuTest, ActivatingAwayFromItemsDoesNothing) {
  MockFunction<void()> func;
  Menu menu = MakeMenu(&func);
  menu.Toggle();

  EXPECT_CALL(func, Call).Times(0);
  // The top-left corner is well outside every centered row's clickable box.
  menu.Activate(/*mouse_x=*/0, /*mouse_y=*/0);
}

TEST(MenuTest, ActivatingWhileClosedDoesNothing) {
  MockFunction<void()> func;
  Menu menu = MakeMenu(&func);
  // The menu is left closed.

  EXPECT_CALL(func, Call).Times(0);
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/0, /*count=*/2));
}

TEST(CheatMenuTest, TitleAtRootIsTheMenuTitle) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();

  EXPECT_STREQ(menu.title(), "FateHacks");
}

TEST(CheatMenuTest, TitleInsideASubmenuIsTheItemDescendedInto) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/1, menu.item_count()));

  EXPECT_STREQ(menu.title(), "More");
}

TEST(CheatMenuTest, LabelAtNamesTheCurrentLevelItem) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/1, menu.item_count()));

  EXPECT_STREQ(menu.label_at(0), "Add Gold x10");
}

TEST(CheatMenuTest, ActivatingAddGoldGivesHundredGold) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();

  EXPECT_CALL(actions, GiveGold(100)).Times(1);
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/0, menu.item_count()));
}

TEST(CheatMenuTest, ActivatingAddGoldXTenGivesThousandGold) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/1, menu.item_count()));

  EXPECT_CALL(actions, GiveGold(100)).Times(10);
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/0, menu.item_count()));
}

TEST(CheatMenuTest, BackFromSubmenuReturnsToRoot) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();
  menu.Activate(Menu::kCenterX, Menu::RowY(/*index=*/1, menu.item_count()));

  menu.Back();

  EXPECT_EQ(menu.item_count(), 2u);
}

}  // namespace
