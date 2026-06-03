#include "ast.hpp"

#include <cstdio>
#include <iostream>
#include <stdexcept>


extern int yyparse();
extern FILE* yyin;
extern Program* g_program;


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: robot_interpreter <program.robot>" << std::endl;
        return 1;
    }

    const char* programPath = argv[1];

    yyin = std::fopen(programPath, "r");

    if (!yyin) {
        std::cerr << "Cannot open file: " << programPath << std::endl;
        return 1;
    }

    int parseResult = yyparse();

    std::fclose(yyin);

    if (parseResult != 0) {
        std::cerr << "Parsing failed" << std::endl;
        return 1;
    }

    if (g_program == nullptr) {
        std::cerr << "Program was not built" << std::endl;
        return 1;
    }

    try {
        RobotClient robotClient;
        g_program->run(robotClient);

        delete g_program;
        g_program = nullptr;
    } catch (const std::exception& error) {
        std::cerr << "Runtime error: " << error.what() << std::endl;

        delete g_program;
        g_program = nullptr;

        return 1;
    }

    return 0;
}