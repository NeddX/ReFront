#include <fstream>
#include <iostream>

#include <CommonDef.h>

#include "Analyzer/Parser.h"
#include "Compiler/Compiler.h"

#include <nlohmann/json.hpp>

using namespace cmm::cmc;

int main(int argc, const char* argv[])
{
    if (argc > 1)
    {
        std::ifstream fs(argv[1]);
        if (fs.is_open())
        {
            auto src    = std::string((std::istreambuf_iterator<char>(fs)), (std::istreambuf_iterator<char>()));
            auto parser = Parser(src);
            auto tree   = parser.Parse();
            nlohmann::ordered_json json = tree;
            std::cout << std::setw(4) << json << std::endl;
            auto compiler      = Compiler(tree);
            auto compiled_code = compiler.Compile();
            compiled_code.size();
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
