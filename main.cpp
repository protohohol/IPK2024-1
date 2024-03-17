#include "CommandLineParser.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Not enough arguments!" << std::endl;
    }

    auto config = CommandLineParser::parseArguments(argc, argv);
    if (!config.valid) {
        std::cerr << "Failed to parse arguments!" << std::endl;
        CommandLineParser::printUsage();
        return 1;
    }

    if (config.show_help) {
        CommandLineParser::printUsage();
        return 0;
    }

    CommandLineParser::printConfig(config);

    return 0;
}