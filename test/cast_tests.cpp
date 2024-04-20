#include "gtest/gtest.h" // cppcheck-suppress [missingInclude]

#include <algorithm>
#include <ranges>
#include <vector>

#include "LWCLI/cast.hpp"

TEST(lwcli_tests, hello)
{
    GTEST_ASSERT_TRUE(true);
}

template<class T>
struct CastTests : public testing::TestWithParam<const char*>
{};

using IntCastTests = CastTests<int>;
using DoubleCastTests = CastTests<double>;
using StringCastTest = CastTests<std::string>;

INSTANTIATE_TEST_SUITE_P(valid_str_ints, IntCastTests, testing::Values("10", "0", "-26"));

TEST_P(IntCastTests, happy_int_casts)
{
    int value;
    ASSERT_NO_THROW({ value = lwcli::cast<int>::from_string(GetParam()); });
    ASSERT_EQ(value, std::stoi(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(valid_str_doubles, DoubleCastTests, testing::Values("10", "0.0", "-26.43"));

TEST_P(DoubleCastTests, happy_double_casts)
{
    double value;
    ASSERT_NO_THROW({ value = lwcli::cast<double>::from_string(GetParam()); });
    ASSERT_EQ(value, std::stod(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    valid_str_strings,
    StringCastTest,
    testing::Values("10", "", "--some-value", "-f", "something-else"));

TEST_P(StringCastTest, happy_string_casts)
{
    std::string value;
    ASSERT_NO_THROW({ value = lwcli::cast<std::string>::from_string(GetParam()); });
    ASSERT_EQ(value, std::string(GetParam()));
}

TEST(IntListCastTest, empty_vector_test)
{
    constexpr auto test_case = "";

    using vec_t = std::vector<int>;
    vec_t result;
    ASSERT_NO_THROW({ result = lwcli::cast<vec_t>::from_string(test_case); });
    ASSERT_EQ(0, result.size());
}

TEST(IntListCastTest, filled_vector_test)
{
    using vec_t = std::vector<int>;
    const vec_t test_case = {10, 20, 30, 40, 50};

    std::stringstream ss;
    ss << test_case.front();
    for (const auto& value : test_case | std::ranges::views::drop(1))
        ss << ", " << value;

    vec_t result;
    ASSERT_NO_THROW({ result = lwcli::cast<vec_t>::from_string(ss.str()); });
    ASSERT_EQ(test_case, result);
}