#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include "AppConfig.h"
#include "Messages.h"

enum class MessageType {
    ERR,
    BYE,
    MSG,
    REPLY,
    UNKNOWN
};

class ChatClient {
private:
    AppConfig config;
    int server_socket;
    std::string username, display_name, secret;
    bool waiting_for_response, opened, bye;

    bool sendMessage(const std::string& message);
    std::string receiveMessage();

    bool sendAuthMessage(const std::string& username, const std::string& display_name, const std::string& secret);
    bool sendJoinMessage(const std::string& channelID, const std::string& display_name);
    bool sendMsgMessage(const std::string& display_name, const std::string& content);
    bool sendByeMessage();
    void printHelp();

    void processCommand(const std::string& input);

    MessageType determineMessageType(const std::string& message);

public:
    ChatClient(const AppConfig& config);
    ~ChatClient();

    void runCLI();
    bool connectToServer();
    void closeConnection();
};

#endif // CHATCLIENT_H
