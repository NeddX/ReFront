#include <fstream>
#include <iostream>

#include <CommonDef.h>

#include "Analyzer/Lexer.h"

using namespace cmm::cmc;

int main(int argc, const char* argv[])
{
    if (argc > 1)
    {
        std::ifstream fs(argv[1]);
        if (fs.is_open())
        {
            auto src   = std::string((std::istreambuf_iterator<char>(fs)), (std::istreambuf_iterator<char>()));
            auto lexer = Lexer(src);
            for (auto tok = lexer.NextToken(); tok.has_value(); tok = lexer.NextToken())
            {
                std::cout << *tok << std::endl;
            }
        }
        else
        {
            std::cerr << "cmc: input file non-existent." << std::endl;
            return -1;
        }
    }
    else
        std::cout << "Usage:\n\tcmc [file]" << std::endl;
    return 0;
}
