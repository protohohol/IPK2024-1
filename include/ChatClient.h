#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include "AppConfig.h"
#include "Messages.h"

enum class MessageType {
    CONFIRM,
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
    uint16_t mid;
    std::string username, display_name, secret;
    bool waiting_for_response, waiting_for_confirm, bye, err, tcp, connect_err, waiting_for_auth;
    struct sockaddr_in * their_addr;

    const std::chrono::milliseconds TIMEOUT;
    const int MAX_RETRIES;

    TimedMessage pending;

    bool sendMessage(const std::string& message);
    bool sendMessage(const std::vector<uint8_t>& message); // For UDP
    void receiveMessage();
    void processMessage(const std::string& message);
    void processMessage(const std::vector<uint8_t>& message);

    bool sendAuthMessage(const std::string& username, const std::string& display_name, const std::string& secret);
    bool sendJoinMessage(const std::string& channelID, const std::string& display_name);
    bool sendMsgMessage(const std::string& display_name, const std::string& content);
    bool sendByeMessage();
    bool sendConfirmMessage(const uint16_t message_id);
    void printHelp();

    void processCommand(const std::string& input);

    MessageType determineMessageType(const std::string& message);
    MessageType determineMessageType(const std::vector<uint8_t>& message);

    bool checkForTimeouts();

public:
    ChatClient(const AppConfig& config);
    ~ChatClient();

    int runCLI();
    bool connectToServer();
    void closeConnection();
};

#endif // CHATCLIENT_H