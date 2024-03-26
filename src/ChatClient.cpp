#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <poll.h>
#include <queue>

#include "ChatClient.h"
#include "ValidationHelpers.h"

ChatClient::ChatClient(const AppConfig& config) : config(config), server_socket(-1), waiting_for_response(false), opened(false), bye(false) {}

ChatClient::~ChatClient() {
    closeConnection();
}

MessageType ChatClient::determineMessageType(const std::string& message) {
    std::istringstream ss(message);
    std::string command;
    ss >> command; // Extracts the first word from the message
    
    if (command == "ERR") {
        return MessageType::ERR;
    } else if (command == "BYE") {
        return MessageType::BYE;
    } else if (command == "MSG") {
        return MessageType::MSG;
    } else if (command == "REPLY") {
        return MessageType::REPLY;
    }
    return MessageType::UNKNOWN;
}


bool ChatClient::connectToServer() {
    struct addrinfo hints = {0}, *addrs, *addr;
    int status;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = this->config.transport_protocol == "tcp" ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_protocol = this->config.transport_protocol == "tcp" ? IPPROTO_TCP : IPPROTO_UDP;
    
    if ((status = getaddrinfo(this->config.server_address.c_str(), std::to_string(this->config.port).c_str(), &hints, &addrs)) != 0) {
        std::cerr << "ERR: getaddrinfo: " << gai_strerror(status) << std::endl;
        return false;
    }

    for (addr = addrs; addr != nullptr; addr = addr->ai_next)
    {
        if ((server_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol)) == -1)
        {
            std::cerr << "ERR: client: socket" << std::endl;
            continue;
        }

        if (connect(server_socket, addr->ai_addr, addr->ai_addrlen) == -1)
        {
            close(server_socket);
            std::cerr << "ERR: client: connect" << std::endl;
            continue;
        }

        break;
    }

    if (addr == nullptr) {
        fprintf(stderr, "ERR: client: failed to connect\n");
        return false;
    }

    freeaddrinfo(addrs);

    return true;
}

bool ChatClient::sendMessage(const std::string& message) {
    ssize_t len = message.size(), sent = 0;

    while (sent < len) {
        ssize_t bytes = send(server_socket, message.data() + sent, len - sent, 0);
        if (bytes < 0) return false; // Send failed
        sent += bytes;
    }

    return true; // Message sent successfully
}

bool ChatClient::sendByeMessage() {
    ByeMessage bye_message;
    return sendMessage(bye_message.serialize());
}

bool ChatClient::sendAuthMessage(const std::string& username, const std::string& display_name, const std::string& secret) {
    AuthMessage auth_message{username, display_name, secret};
    return sendMessage(auth_message.serialize());
}

bool ChatClient::sendJoinMessage(const std::string& channelID, const std::string& display_name) {
    JoinMessage join_message{channelID, display_name};
    return sendMessage(join_message.serialize());
}

bool ChatClient::sendMsgMessage(const std::string& display_name, const std::string& content) {
    MsgMessage msg_message{display_name, content};
    return sendMessage(msg_message.serialize());
}

void ChatClient::runCLI() {
    // Setup poll structure for stdin and the server socket
    struct pollfd fds[2];
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;  // Check for incoming data
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;  // Check for input from the terminal

    std::queue<std::string> command_queue;  // Queue to hold commands when waiting for a server response

    while (true) {
        if (bye) {
            std::cerr << "ERR: Received BYE message. Exiting..." << std::endl;
            break; // Exit the loop to terminate the program
        }


        int ret = poll(fds, 2, -1);  // Wait indefinitely until there's activity
        if (ret == -1) {
            // Handle error
            perror("poll");
            break;
        }

        // Check for incoming messages from the server
        if (fds[0].revents & POLLIN) {
            std::string message = receiveMessage();
            // Process message, potentially clearing the waitingForResponse flag
            // depending on your application's protocol
        }

        // Check for user input from stdin
        if (fds[1].revents & POLLIN) {
            std::string input;
            if (std::getline(std::cin, input)) {
                std::istringstream iss(input);
                std::string command;
                std::getline(iss, command, ' '); // Extract the command part of the input

                // Directly handle /rename and /help commands even if waiting for a response
                if (command == "/rename" || command == "/help") {
                    processCommand(input);
                } else if (waiting_for_response) {
                    std::cerr << "ERR: waiting for response from server" << std::endl;

                    // Queue other commands when waiting for a response
                    command_queue.push(input);
                } else {
                    // Process other commands immediately if not waiting for a response
                    processCommand(input);
                }
            }
        }

        // If not waiting for a response and there are queued commands, process the next one
        if (!waiting_for_response && !command_queue.empty()) {
            std::string next_command = command_queue.front();
            command_queue.pop();
            processCommand(next_command);
        }
    }
}

// Example implementation of processCommand (you need to implement it based on your needs)
void ChatClient::processCommand(const std::string& input) {
        std::istringstream iss(input);

        std::string command;
        std::getline(iss, command, ' '); // Extract the command part of the input

        std::vector<std::string> params;
        std::string param;
        while (std::getline(iss, param, ' ')) {
            params.push_back(param);
        }

        if (command == "/auth" && params.size() == 3 && isValidId(params[0]) && isValidSecret(params[1]) && isValidDName(params[2])) {
            if (sendAuthMessage(params[0], params[1], params[2])) {
                username = params[0];
                secret = params[1];
                display_name = params[2];
                waiting_for_response = true; // Assuming a response is needed
            }
        } else if (command == "/join" && params.size() == 1 && isValidId(params[0])) {
            if (sendJoinMessage(params[0], display_name)) {
                waiting_for_response = true; // Assuming a response is needed
            }
        } else if (command == "/rename" && params.size() == 1 && isValidDName(params[0])) {
            display_name = params[0]; // Update the display name
        } else if (command == "/help") {
            printHelp();
        } else {
            if (input[0] == '/') {
                std::cerr << "ERR: Invalid command or parameter(s). ||" << input << "||" << std::endl;
            } else {
                sendMsgMessage(display_name, input);
            }

            // std::cerr << "ERR: Invalid command or parameter(s)." << std::endl;
            // printHelp();
        }
}

std::string ChatClient::receiveMessage() {
    char buffer[1024] = {};
    ssize_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);

    std::string message;
    if (bytes_received > 0) {
        message.assign(buffer, bytes_received);
    } else {
        return {}; // Return empty string if no data
    }

    if (!message.empty()) {
        MessageType type = determineMessageType(message);
        switch (type) {
            case MessageType::ERR:
                {
                    ErrorMessage msg = ErrorMessage::deserialize(message);

                    std::cerr << "ERR FROM " << msg.display_name << ": " << msg.message_content << std::endl;

                    break;
                }
            case MessageType::BYE:
                bye = true;
                break;
            case MessageType::MSG:
                {
                    MsgMessage msg = MsgMessage::deserialize(message);

                    std::cout << msg.display_name << ": " << msg.message_content << std::endl;

                    break;
                }
            case MessageType::REPLY:
                {
                    ReplyMessage msg = ReplyMessage::deserialize(message);

                    std::cerr << (msg.success ? "Success: " : "Failure: ") << msg.message_content << std::endl;

                    waiting_for_response = false;

                    break;
                }
            case MessageType::UNKNOWN:
            default:
                std::cerr << "ERR: Unknown or malformed message received. ||" << message <<  "||" << std::endl;
                break;
        }
    }

    return message;
}

void ChatClient::printHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "/auth <Username> <Secret> <DisplayName> - Authenticate with the server." << std::endl;
    std::cout << "/join <ChannelID> - Join a chat channel." << std::endl;
    std::cout << "/rename <DisplayName> - Change your display name." << std::endl;
    std::cout << "/help - Show help message." << std::endl;
}

void ChatClient::closeConnection() {
    if (!bye) sendByeMessage();

    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
}