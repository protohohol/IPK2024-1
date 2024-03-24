#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>

#include "ChatClient.h"
#include "ValidationHelpers.h"

ChatClient::ChatClient(const AppConfig& config) : config(config), server_socket(-1) {}

ChatClient::~ChatClient() {
    closeConnection();
}

bool ChatClient::connectToServer() {
    struct addrinfo hints = {0}, *addrs, *addr;
    int status;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = this->config.transport_protocol == "tcp" ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_protocol = this->config.transport_protocol == "tcp" ? IPPROTO_TCP : IPPROTO_UDP;
    
    if ((status = getaddrinfo(this->config.server_address.c_str(), std::to_string(this->config.port).c_str(), &hints, &addrs)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return false;
    }

    for (addr = addrs; addr != nullptr; addr = addr->ai_next)
    {
        if ((server_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol)) == -1)
        {
            std::cerr << "client: socket" << std::endl;
            continue;
        }

        if (connect(server_socket, addr->ai_addr, addr->ai_addrlen) == -1)
        {
            close(server_socket);
            std::cerr << "client: connect" << std::endl;
            continue;
        }

        break;
    }

    if (addr == nullptr) {
        fprintf(stderr, "client: failed to connect\n");
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

bool ChatClient::sendAuthMessage(const std::string& username, const std::string& display_name, const std::string& secret) {
    AuthMessage authMessage{username, display_name, secret};
    return sendMessage(authMessage.serialize());
}

bool ChatClient::sendJoinMessage(const std::string& channelID, const std::string& display_name) {
    JoinMessage joinMessage{channelID, display_name};
    return sendMessage(joinMessage.serialize());
}

void ChatClient::runCLI() {
    std::string input;
    while (std::getline(std::cin, input)) {
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
                this->username = params[0];
                this->secret = params[1];
                this->display_name = params[2];
            }
        } else if (command == "/join" && params.size() == 1 && isValidId(params[0])) {
            sendJoinMessage(params[0], this->display_name);
        } else if (command == "/rename" && params.size() == 1 && isValidDName(params[0])) {
            this->display_name = params[0]; // Update the display name
        } else if (command == "/help") {
            printHelp();
        } else {
            std::cout << "Invalid command or parameter(s)." << std::endl;
            printHelp();
        }
    }
}

void ChatClient::printHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "/auth <Username> <Secret> <DisplayName> - Authenticate with the server." << std::endl;
    std::cout << "/join <ChannelID> - Join a chat channel." << std::endl;
    std::cout << "/rename <DisplayName> - Change your display name." << std::endl;
    std::cout << "/help - Show help message." << std::endl;
}

void ChatClient::closeConnection() {
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
}

std::string ChatClient::receiveMessage() {
    char buffer[1024] = {};
    ssize_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) return std::string(buffer, bytes_received);
    return {}; // Return empty string if no data
}
