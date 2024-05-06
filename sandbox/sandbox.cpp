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

    lwcli::KeyValueOption<double> op2;
    op2.aliases = {"-t", "--test1"};
    op2.description = "This sets some other value";

    lwcli::CLIParser parser;
    parser.register_option(op1);
    parser.register_option(op2);

    try {
        parser.parse(argc, argv);
    }
    catch (const lwcli::bad_parse& e) {
        std::cout << e.what();
    }
}
