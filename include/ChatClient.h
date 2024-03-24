#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include "AppConfig.h"
#include "Messages.h"

class ChatClient {
private:
    AppConfig config;
    int server_socket;
    std::string username, display_name, secret;

public:
    ChatClient(const AppConfig& config);
    ~ChatClient();

    bool connectToServer();
    bool sendMessage(const std::string& message);
    std::string receiveMessage();
    void closeConnection();

    bool sendAuthMessage(const std::string& username, const std::string& display_name, const std::string& secret);
    bool sendJoinMessage(const std::string& channelID, const std::string& display_name);
    void runCLI();  // Interactive CLI for user commands
    void printHelp();
};

#endif // CHATCLIENT_H
