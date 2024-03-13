#include <vector>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

#include "ppr/common/level.h"
#include "ppr/preprocessing/osm/level.h"

using namespace ppr;
using namespace ppr::preprocessing::osm;
using namespace testing;

TEST(LevelParserTest, NoLevelTag) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels(nullptr, levels_vec);
  EXPECT_FALSE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
}

TEST(LevelParserTest, EmptyLevelTag) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("", levels_vec);
  EXPECT_FALSE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
}

TEST(LevelParserTest, WhitespaceOnlyLevelTag) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels(" ", levels_vec);
  EXPECT_FALSE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
}

TEST(LevelParserTest, SingleLevel0) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("0", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  ASSERT_TRUE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
  EXPECT_EQ(to_human_level(lvl.single_level()), 0);
}

TEST(LevelParserTest, SingleLevel1) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("1", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  ASSERT_TRUE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
  EXPECT_EQ(to_human_level(lvl.single_level()), 1);
}

TEST(LevelParserTest, SingleLevelMinus1) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("-1", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  ASSERT_TRUE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
  EXPECT_EQ(to_human_level(lvl.single_level()), -1);
}

TEST(LevelParserTest, SingleLevelFractional) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("1.5", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  ASSERT_TRUE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
  EXPECT_EQ(to_human_level(lvl.single_level()), 1.5);
}

TEST(LevelParserTest, SingleLevelNegativeFractional) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("-0.5", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  ASSERT_TRUE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
  EXPECT_EQ(to_human_level(lvl.single_level()), -0.5);
}

TEST(LevelParserTest, MultiLevelSimple01) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("0;1", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  ASSERT_TRUE(lvl.has_multiple_levels());
  EXPECT_THAT(levels_vec.at(lvl.multi_level_index()),
              ElementsAre(unchecked_from_human_level(0),
                          unchecked_from_human_level(1)));
}

TEST(LevelParserTest, MultiLevelSimple10) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("1;0", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  ASSERT_TRUE(lvl.has_multiple_levels());
  EXPECT_THAT(levels_vec.at(lvl.multi_level_index()),
              ElementsAre(unchecked_from_human_level(1),
                          unchecked_from_human_level(0)));
}

TEST(LevelParserTest, MultiLevelSimple01WithWhitespace) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("0; 1", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  ASSERT_TRUE(lvl.has_multiple_levels());
  EXPECT_THAT(levels_vec.at(lvl.multi_level_index()),
              ElementsAre(unchecked_from_human_level(0),
                          unchecked_from_human_level(1)));
}

TEST(LevelParserTest, MultiLevelRange) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("2-4", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  ASSERT_TRUE(lvl.has_multiple_levels());
  EXPECT_THAT(
      levels_vec.at(lvl.multi_level_index()),
      ElementsAre(unchecked_from_human_level(2), unchecked_from_human_level(3),
                  unchecked_from_human_level(4)));
}

TEST(LevelParserTest, MultiLevelRangeWithWhitespace) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("2 - 4", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  ASSERT_TRUE(lvl.has_multiple_levels());
  EXPECT_THAT(
      levels_vec.at(lvl.multi_level_index()),
      ElementsAre(unchecked_from_human_level(2), unchecked_from_human_level(3),
                  unchecked_from_human_level(4)));
}

TEST(LevelParserTest, MultiLevelComplex1) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("-2;-1; 0-31", levels_vec);
  EXPECT_TRUE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  ASSERT_TRUE(lvl.has_multiple_levels());
  auto expected = std::vector{unchecked_from_human_level(-2),
                              unchecked_from_human_level(-1)};
  for (auto i = 0; i <= 31; ++i) {
    expected.emplace_back(unchecked_from_human_level(i));
  }
  EXPECT_THAT(levels_vec.at(lvl.multi_level_index()),
              ElementsAreArray(expected));
}

TEST(LevelParserTest, Invalid1) {
  auto levels_vec = levels_vector_t{};
  auto const lvl = get_levels("HC III", levels_vec);
  EXPECT_FALSE(lvl.has_level());
  EXPECT_FALSE(lvl.has_single_level());
  EXPECT_FALSE(lvl.has_multiple_levels());
}
