#include <iostream>

#include <CommonDef.h>

#include "Analyzer/Lexer.h"

using namespace cmm::cmc;

int main()
{
    auto lexer = Lexer("int a = 10;");
    for (auto tok = lexer.NextToken(); tok.has_value(); tok = lexer.NextToken())
    {
        std::cout << *tok << std::endl;
    }
    return 0;
}
