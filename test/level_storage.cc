#include <vector>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

#include "ppr/common/level.h"

using namespace ppr;
using namespace testing;

TEST(LevelStorageTest, NoLevel) {
  auto const lvl = levels{};
  EXPECT_FALSE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
}

TEST(LevelStorageTest, SingleLevel) {
  for (auto const human_level :
       {-2.0, -1.5, -0.5, 0.0, 0.5, 1.0, 1.5, 2.0, 123.0}) {
    auto const level = from_human_level(human_level);
    auto const lvl = make_single_level(level);
    EXPECT_TRUE(lvl.has_level());
    EXPECT_TRUE(lvl.has_single_level());
    EXPECT_FALSE(lvl.has_multiple_levels());
    EXPECT_EQ(lvl.single_level(), level);
  }
}

TEST(LevelStorageTest, MultiLevel) {
  auto levels_vec = levels_vector_t{};
  auto const lvls = std::vector{*from_human_level(0.0), *from_human_level(1.0)};
  auto const levels_idx = levels_vec.size();
  levels_vec.emplace_back(lvls);

  auto const lvl = make_multiple_levels(levels_idx);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  EXPECT_TRUE(lvl.has_multiple_levels());
  EXPECT_EQ(lvl.multi_level_index(), levels_idx);
  EXPECT_THAT(levels_vec.at(lvl.multi_level_index()), ElementsAreArray(lvls));
}

TEST(LevelStorageTest, LevelConversion) {
  for (auto const human_level :
       {-2.0, -1.5, -0.5, 0.0, 0.5, 1.0, 1.5, 2.0, 123.0}) {
    auto const level = from_human_level(human_level);
    EXPECT_EQ(to_human_level(*level), human_level);
  }
}

TEST(LevelStorageTest, MatchesLevelNoLevel) {
  auto const levels_vec = levels_vector_t{};
  auto const lvl = levels{};
  EXPECT_FALSE(matches_level(levels_vec, lvl, 10, false));
  EXPECT_TRUE(matches_level(levels_vec, lvl, 10, true));
}

TEST(LevelStorageTest, MatchesLevelSingleLevel) {
  auto const levels_vec = levels_vector_t{};
  auto const level = *from_human_level(1);
  auto const lvl = make_single_level(level);
  EXPECT_TRUE(matches_level(levels_vec, lvl, level, false));
  EXPECT_TRUE(matches_level(levels_vec, lvl, level, true));
  EXPECT_FALSE(matches_level(levels_vec, lvl, 0, false));
  EXPECT_FALSE(matches_level(levels_vec, lvl, 0, true));
}

TEST(LevelStorageTest, MatchesLevelMultiLevel) {
  auto levels_vec = levels_vector_t{};
  auto const lvls = std::vector{*from_human_level(0.0), *from_human_level(1.0)};
  auto const levels_idx = levels_vec.size();
  levels_vec.emplace_back(lvls);
  auto const lvl = make_multiple_levels(levels_idx);

  for (auto const& level : lvls) {
    EXPECT_TRUE(matches_level(levels_vec, lvl, level, false));
    EXPECT_TRUE(matches_level(levels_vec, lvl, level, true));
  }
  EXPECT_FALSE(matches_level(levels_vec, lvl, 500, false));
  EXPECT_FALSE(matches_level(levels_vec, lvl, 500, true));
}

TEST(LevelStorageTest, HaveSharedLevelEmptyEmpty) {
  auto const levels_vec = levels_vector_t{};
  auto const a = levels{};
  auto const b = levels{};
  EXPECT_FALSE(have_shared_level(levels_vec, a, b, false));
  EXPECT_TRUE(have_shared_level(levels_vec, a, b, true));
}

TEST(LevelStorageTest, HaveSharedLevelEmptySingle) {
  auto const levels_vec = levels_vector_t{};
  auto const a = levels{};
  auto const b = make_single_level(*from_human_level(1));
  EXPECT_FALSE(have_shared_level(levels_vec, a, b, false));
  EXPECT_FALSE(have_shared_level(levels_vec, b, a, false));
  EXPECT_TRUE(have_shared_level(levels_vec, a, b, true));
  EXPECT_TRUE(have_shared_level(levels_vec, b, a, true));
}

TEST(LevelStorageTest, HaveSharedLevelSingleSingle) {
  auto const levels_vec = levels_vector_t{};
  auto const a = make_single_level(*from_human_level(0));
  auto const b = make_single_level(*from_human_level(1));
  auto const c = make_single_level(*from_human_level(1));

  EXPECT_FALSE(have_shared_level(levels_vec, a, b, false));
  EXPECT_FALSE(have_shared_level(levels_vec, b, a, false));
  EXPECT_FALSE(have_shared_level(levels_vec, a, b, true));
  EXPECT_FALSE(have_shared_level(levels_vec, b, a, true));

  EXPECT_TRUE(have_shared_level(levels_vec, b, c, false));
  EXPECT_TRUE(have_shared_level(levels_vec, b, c, true));
}

TEST(LevelStorageTest, HaveSharedLevelEmptyMulti) {
  auto levels_vec = levels_vector_t{};
  auto const lvls = std::vector{*from_human_level(0.0), *from_human_level(1.0)};
  auto const levels_idx = levels_vec.size();
  levels_vec.emplace_back(lvls);

  auto const a = levels{};
  auto const b = make_multiple_levels(levels_idx);

  EXPECT_FALSE(have_shared_level(levels_vec, a, b, false));
  EXPECT_FALSE(have_shared_level(levels_vec, b, a, false));
  EXPECT_TRUE(have_shared_level(levels_vec, a, b, true));
  EXPECT_TRUE(have_shared_level(levels_vec, b, a, true));
}

TEST(LevelStorageTest, HaveSharedLevelSingleMulti) {
  auto levels_vec = levels_vector_t{};
  auto const lvls = std::vector{*from_human_level(0.0), *from_human_level(1.0)};
  auto const levels_idx = levels_vec.size();
  levels_vec.emplace_back(lvls);

  auto const a = make_multiple_levels(levels_idx);
  auto const b = make_single_level(lvls[0]);
  auto const c = make_single_level(500);

  EXPECT_TRUE(have_shared_level(levels_vec, a, b, false));
  EXPECT_TRUE(have_shared_level(levels_vec, b, a, false));
  EXPECT_TRUE(have_shared_level(levels_vec, a, b, true));
  EXPECT_TRUE(have_shared_level(levels_vec, b, a, true));

  EXPECT_FALSE(have_shared_level(levels_vec, a, c, false));
  EXPECT_FALSE(have_shared_level(levels_vec, c, a, false));
  EXPECT_FALSE(have_shared_level(levels_vec, a, c, true));
  EXPECT_FALSE(have_shared_level(levels_vec, c, a, true));
}

TEST(LevelStorageTest, HaveSharedLevelMultiMulti) {
  auto levels_vec = levels_vector_t{};
  auto const a_lvls =
      std::vector{*from_human_level(0.0), *from_human_level(1.0)};
  auto const b_lvls =
      std::vector{*from_human_level(-1.0), *from_human_level(0.0)};
  auto const c_lvls =
      std::vector{*from_human_level(2.0), *from_human_level(3.0)};
  levels_vec.emplace_back(a_lvls);
  levels_vec.emplace_back(b_lvls);
  levels_vec.emplace_back(c_lvls);

  auto const a = make_multiple_levels(0);
  auto const b = make_multiple_levels(1);
  auto const c = make_multiple_levels(2);

  EXPECT_TRUE(have_shared_level(levels_vec, a, b, false));
  EXPECT_TRUE(have_shared_level(levels_vec, b, a, false));
  EXPECT_TRUE(have_shared_level(levels_vec, a, b, true));
  EXPECT_TRUE(have_shared_level(levels_vec, b, a, true));

  EXPECT_FALSE(have_shared_level(levels_vec, a, c, false));
  EXPECT_FALSE(have_shared_level(levels_vec, c, a, false));
  EXPECT_FALSE(have_shared_level(levels_vec, a, c, true));
  EXPECT_FALSE(have_shared_level(levels_vec, c, a, true));
}

TEST(LevelStorageTest, ClosestLevelEmpty) {
  auto const levels_vec = levels_vector_t{};
  auto const lvl = levels{};
  EXPECT_EQ(closest_level(levels_vec, lvl, 0), std::nullopt);
}

TEST(LevelStorageTest, ClosestLevelSingle) {
  auto const levels_vec = levels_vector_t{};
  auto const level = *from_human_level(1);
  auto const lvl = make_single_level(level);
  EXPECT_THAT(closest_level(levels_vec, lvl, 0), Optional(level));
}

TEST(LevelStorageTest, ClosestLevelMulti) {
  auto levels_vec = levels_vector_t{};
  auto const lvls = std::vector{*from_human_level(0.0), *from_human_level(1.0)};
  auto const levels_idx = levels_vec.size();
  levels_vec.emplace_back(lvls);
  auto const lvl = make_multiple_levels(levels_idx);

  EXPECT_THAT(closest_level(levels_vec, lvl, *from_human_level(-1.0)),
              Optional(*from_human_level(0.0)));
  EXPECT_THAT(closest_level(levels_vec, lvl, *from_human_level(0.0)),
              Optional(*from_human_level(0.0)));
  EXPECT_THAT(closest_level(levels_vec, lvl, *from_human_level(1.0)),
              Optional(*from_human_level(1.0)));
  EXPECT_THAT(closest_level(levels_vec, lvl, *from_human_level(2.0)),
              Optional(*from_human_level(1.0)));
}
