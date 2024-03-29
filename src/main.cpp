#include <cstdlib>
#include <csignal>
#include <iostream>

#include "CommandLineParser.h"
#include "ChatClient.h"

ChatClient* client = nullptr; // Global pointer to ChatClient instance

void handle_sigint(int sig) {
    if (client != nullptr) {
        std::cerr << "ERR: Ctrl+C pressed. Shutting down..." << std::endl;
        delete client;
        client = nullptr;
    }

    return;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handle_sigint); // Register signal handler
    
    if (argc < 2) {
        std::cerr << "ERR: Not enough arguments!" << std::endl;
        return EXIT_FAILURE;
    }

    AppConfig config = CommandLineParser::parseArguments(argc, argv);
    if (!config.valid) {
        std::cerr << "ERR: Failed to parse arguments!" << std::endl;
        CommandLineParser::printUsage();
        return EXIT_FAILURE;
    }

    if (config.show_help) {
        CommandLineParser::printUsage();
        return EXIT_FAILURE;
    }

    client = new ChatClient(config);
    if (!client->connectToServer()) {
        std::cerr << "ERR: Could not connect to the server." << std::endl;
        delete client;
        client = nullptr;
        return EXIT_FAILURE;
    }

    client->runCLI();

    delete client;
    client = nullptr;

    return EXIT_SUCCESS;
}