#include "gtest/gtest.h"

#include "LWCLI/cast.hpp"
#include <vector>

TEST(lwcli_tests, hello)
{
    GTEST_ASSERT_TRUE(true);
}

template<class T>
struct CastTests : public testing::TestWithParam<const char*> {};

using IntCastTests = CastTests<int>;
using DoubleCastTests = CastTests<double>;
using StringCastTest = CastTests<std::string>;

INSTANTIATE_TEST_SUITE_P(
    valid_str_ints,
    IntCastTests,
    testing::Values("10", "0", "-26"));

INSTANTIATE_TEST_SUITE_P(
    valid_str_doubles,
    DoubleCastTests,
    testing::Values("10", "0.0", "-26.43"));

INSTANTIATE_TEST_SUITE_P(
    valid_str_strings,
    StringCastTest,
    testing::Values("10", "", "--some-value", "-f", "something-else"));

TEST_P(IntCastTests, happy_int_casts)
{
    int value;
    ASSERT_NO_THROW({ value = lwcli::cast<int>::from_string(GetParam()); });
    ASSERT_EQ(value, std::stoi(GetParam()));
}

TEST_P(DoubleCastTests, happy_double_casts)
{
    double value;
    ASSERT_NO_THROW({ value = lwcli::cast<double>::from_string(GetParam()); });
    ASSERT_EQ(value, std::stod(GetParam()));
}

TEST_P(StringCastTest, happy_string_casts)
{
    std::string value;
    ASSERT_NO_THROW({ value = lwcli::cast<std::string>::from_string(GetParam()); });
    ASSERT_EQ(value, std::string(GetParam()));
}