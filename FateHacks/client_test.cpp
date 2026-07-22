// Tests the game-independent pieces of client.hpp/.cpp -- currently just the
// damage multiplier's own math. client.cpp is compiled here (so its pure
// logic can be tested directly, no inline-in-header workaround needed), but
// nothing in these tests calls the parts that touch real game memory or
// Detours, since those only work with the game running.

#include "client.hpp"

#include "gtest/gtest.h"

namespace {

using ::fate::ApplyDamageMultiplier;

TEST(ApplyDamageMultiplierTest, MultipliesDamageFromThePlayer) {
  EXPECT_FLOAT_EQ(ApplyDamageMultiplier(/*damage=*/5.0f,
                                        /*attacker_is_player=*/true,
                                        /*multiplier=*/10),
                  50.0f);
}

TEST(ApplyDamageMultiplierTest, LeavesDamageFromSomeoneElseUnchanged) {
  EXPECT_FLOAT_EQ(ApplyDamageMultiplier(/*damage=*/5.0f,
                                        /*attacker_is_player=*/false,
                                        /*multiplier=*/10),
                  5.0f);
}

TEST(ApplyDamageMultiplierTest, DefaultMultiplierLeavesDamageUnchanged) {
  EXPECT_FLOAT_EQ(ApplyDamageMultiplier(/*damage=*/5.0f,
                                        /*attacker_is_player=*/true,
                                        /*multiplier=*/1),
                  5.0f);
}

}  // namespace
