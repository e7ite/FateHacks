// Tests the game-independent pieces of client.hpp/.cpp. client.cpp is compiled
// here (so its pure logic can be tested directly, no inline-in-header
// workaround needed), but nothing in these tests calls the parts that touch
// real game memory or Detours, since those only work with the game running.

#include "client.hpp"

#include "gtest/gtest.h"

namespace {

using ::fate::AddDamageDealtBonusEffect;
using ::fate::ApplyDamageMultiplier;
using ::fate::CEffect;
using ::fate::CItem;
using ::fate::EEffectType;
using ::fate::FindEffect;

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

TEST(FindEffectTest, FindsTheMatchingEffectAmongOthers) {
  CEffect other{};
  other.type = static_cast<EEffectType>(7);  // Anything but DamageDealtBonus.
  CEffect matching{};
  matching.type = EEffectType::DamageDealtBonus;
  matching.value = 5.0f;
  CEffect* effects[] = {&other, &matching};

  EXPECT_EQ(FindEffect(effects, effects + 2, EEffectType::DamageDealtBonus),
            &matching);
}

TEST(FindEffectTest, SkipsNullEntries) {
  CEffect matching{};
  matching.type = EEffectType::DamageDealtBonus;
  CEffect* effects[] = {nullptr, &matching};

  EXPECT_EQ(FindEffect(effects, effects + 2, EEffectType::DamageDealtBonus),
            &matching);
}

TEST(FindEffectTest, ReturnsNullWhenNoneMatch) {
  CEffect other{};
  other.type = static_cast<EEffectType>(7);  // Anything but DamageDealtBonus.
  CEffect* effects[] = {&other};

  EXPECT_EQ(FindEffect(effects, effects + 1, EEffectType::DamageDealtBonus),
            nullptr);
}

TEST(AddDamageDealtBonusEffectTest, AddsToAnExistingDamageDealtBonusDirectly) {
  // When the item already has a Damage Dealt Bonus effect this is a plain field
  // write -- it never allocates or calls into the game, so it's safe to run
  // outside the game process.
  CEffect damage_dealt_bonus{};
  damage_dealt_bonus.type = EEffectType::DamageDealtBonus;
  damage_dealt_bonus.value = 5.0f;
  CEffect* effects[] = {&damage_dealt_bonus};
  CItem item{};
  item.effects[0].begin = effects;
  item.effects[0].end = effects + 1;

  AddDamageDealtBonusEffect(&item, /*delta=*/2.0f);

  EXPECT_FLOAT_EQ(damage_dealt_bonus.value, 7.0f);
}

}  // namespace
