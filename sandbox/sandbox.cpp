#include <iostream>

#include "LWCLI/exceptions.hpp"
#include "LWCLI/options.hpp"
#include "LWCLI/parser.hpp"

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv)
{
    lwcli::PositionalOption<int> op1;
    op1.name = "test1";
    op1.description = "This sets some value";

    lwcli::CLIParser parser;
    parser.register_option(op1);

    try {
        parser.parse(argc, argv);
    }
    catch (const lwcli::bad_parse& e) {
        std::cout << e.what();
    }
}
