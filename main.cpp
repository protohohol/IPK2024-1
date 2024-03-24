#include <cstdlib>
#include "CommandLineParser.h"
#include "ChatClient.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Not enough arguments!" << std::endl;
        return EXIT_FAILURE;
    }

    AppConfig config = CommandLineParser::parseArguments(argc, argv);
    if (!config.valid) {
        std::cerr << "Failed to parse arguments!" << std::endl;
        CommandLineParser::printUsage();
        return EXIT_FAILURE;
    }

    if (config.show_help) {
        CommandLineParser::printUsage();
        return EXIT_SUCCESS;
    }

    ChatClient client(config);
    if (!client.connectToServer()) {
        std::cerr << "Could not connect to the server." << std::endl;
        return EXIT_FAILURE;
    }

    client.runCLI();

    client.closeConnection();

    return EXIT_SUCCESS;
}