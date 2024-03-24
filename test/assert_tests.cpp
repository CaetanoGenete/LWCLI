#include "gtest/gtest.h"

#include "LWCLI/LWCLI.hpp"

TEST(assert_tests, multi_options_same_key)
{
    lwcli::FlagOption option1;
    option1.aliases = { "--value1", "--value2" };

    lwcli::FlagOption option2;
    option2.aliases = { "--value2", "--value3" };

    lwcli::CLIParser parser;
    parser.register_option(option1);
    EXPECT_DEATH(parser.register_option(option2), ".*");
}

TEST(assert_tests, multi_options_same_key_different_option_types)
{
    lwcli::FlagOption option1;
    option1.aliases = { "--value1", "--value2" };

    lwcli::KeyValueOption<int> option2;
    option2.aliases = { "--value2", "--value3" };

    lwcli::CLIParser parser;
    parser.register_option(option1);
    EXPECT_DEATH(parser.register_option(option2), ".*");
}