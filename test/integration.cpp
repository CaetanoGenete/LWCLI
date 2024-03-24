#include "gtest/gtest.h"

#include "LWCLI/LWCLI.hpp"

TEST(integration, all_options_types_happy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = { "-v" };

    lwcli::KeyValueOption<int> key_value_option;
    key_value_option.aliases = { "--value" };

    lwcli::PositionalOption<double> positional_option;

    lwcli::CLIParser parser;
    parser.register_option(flag_option);
    parser.register_option(key_value_option);
    parser.register_option(positional_option);

    const char* value = "10";
    const char* positional = "0.31415926";
    const char* argv[] = { "integration", "-v", "--value", value, positional };

    ASSERT_NO_THROW(parser.parse(static_cast<int>(std::size(argv)), argv));
    ASSERT_EQ(1, flag_option.count);
    ASSERT_EQ(std::stoi(value), key_value_option.value);
    ASSERT_EQ(std::stod(positional), positional_option.value);
}

struct key_value_unhappy_tests : testing::TestWithParam<std::vector<const char*>>
{
    static constexpr auto key = "--value";
};

INSTANTIATE_TEST_SUITE_P(
    unhappy_key_value_args,
    key_value_unhappy_tests,
    testing::Values(
        std::vector({ "integration", key_value_unhappy_tests::key }),
        std::vector({ "integration", key_value_unhappy_tests::key, "non-int-value" }),
        std::vector({ "integration", "--undefined-key", "10" })));

TEST_P(key_value_unhappy_tests, key_value_unhappy)
{
    lwcli::KeyValueOption<int> key_value;
    key_value.aliases = { key_value_unhappy_tests::key };

    lwcli::CLIParser parser;
    parser.register_option(key_value);

    const std::vector<const char*>& args = GetParam();
    const auto argc = static_cast<int>(std::size(args));
    const char* const* const argv = std::data(args);
    ASSERT_THROW(parser.parse(argc, argv), lwcli::bad_parse);
}