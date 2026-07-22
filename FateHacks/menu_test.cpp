// Tests the cheat menu: opening and closing it, navigating it, and the
// character cheats each item triggers.

#include "menu.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

// A minimal two-item menu for exercising navigation without any character
// cheat.
Menu MakeMenu() {
  return Menu("Title", Items(Action("One", [] {}), Action("Two", [] {})));
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

TEST(CheatMenuTest, TitleAtRootIsTheMenuTitle) {
  MockCharacterActions actions;
  Menu menu = BuildCheatMenu(&actions);
  menu.Toggle();

  EXPECT_STREQ(menu.title(), "FateHacks");
}

}  // namespace
