#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <poll.h>
#include <queue>
#include <iomanip>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ChatClient.h"
#include "ValidationHelpers.h"

ChatClient::ChatClient(const AppConfig& config)
: config(config), server_socket(-1), mid(0),
  waiting_for_response(false), waiting_for_confirm(false), bye(false), err(false),
  tcp(config.transport_protocol == "tcp"), connect_err(false), waiting_for_auth(true),
  their_addr(nullptr), TIMEOUT(std::chrono::milliseconds(config.timeout)),
  MAX_RETRIES(config.retransmissions_number)  
  {
        their_addr = new struct sockaddr_in;
        memset(their_addr, 0, sizeof(struct sockaddr_in));
  }

ChatClient::~ChatClient() {
    if (!connect_err) closeConnection();

    delete their_addr;

    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1; // Mark as closed.
    }
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

MessageType ChatClient::determineMessageType(const std::vector<uint8_t>& message) {
    if (message.empty()) return MessageType::UNKNOWN;

    switch (message[0]) {
        case 0x00: return MessageType::CONFIRM;
        case 0x01: return MessageType::REPLY;
        case 0x04: return MessageType::MSG;
        case 0xFE: return MessageType::ERR;
        case 0xFF: return MessageType::BYE;
        default:   return MessageType::UNKNOWN;
    }
}

bool ChatClient::connectToServer() {
    if (!tcp) {
        struct hostent * he;
        int broadcast = 1;

        if ((he=gethostbyname(config.server_address.c_str())) == NULL) {  // get the host info
            std::cerr << "ERR: gethostbyname" << std::endl;
            connect_err = true;
            return false;
        }

        if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            std::cerr << "ERR: socket" << std::endl;
            connect_err = true;
            return false;
        }

        // this call is what allows broadcast packets to be sent:
        if (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
            std::cerr << "ERR: setsockopt (SO_BROADCAST)" << std::endl;
            connect_err = true;
            return false;
        }


        their_addr->sin_family = AF_INET;     // host byte order
        their_addr->sin_port = htons(config.port); // short, network byte order
        their_addr->sin_addr = *((struct in_addr *)he->h_addr);
        memset(their_addr->sin_zero, '\0', sizeof their_addr->sin_zero);

        return true;
    } else {
        struct addrinfo hints = {0}, *addrs, *addr;

        int status;

        hints.ai_family = AF_INET;
        hints.ai_socktype = tcp ? SOCK_STREAM : SOCK_DGRAM;
        
        if ((status = getaddrinfo(config.server_address.c_str(), std::to_string(this->config.port).c_str(), &hints, &addrs)) != 0) {
            std::cerr << "ERR: getaddrinfo: " << gai_strerror(status) << std::endl;
            connect_err = true;
            return false;
        }
        
        for (addr = addrs; addr != nullptr; addr = addr->ai_next)
        {
            if ((server_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
            {
                std::cerr << "ERR: client: socket" << std::endl;
                continue;
            }

            if (tcp && connect(server_socket, addr->ai_addr, addr->ai_addrlen) == -1)
            {
                close(server_socket);
                std::cerr << "ERR: client: connect" << std::endl;
                continue;
            }

            break;
        }

        if (addr == nullptr) {
            fprintf(stderr, "ERR: client: failed to connect\n");
            freeaddrinfo(addrs);
            return false;
        }

        freeaddrinfo(addrs);

        return true;
    }
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

bool ChatClient::sendMessage(const std::vector<uint8_t>& message) {
    ssize_t len = message.size();

    // Cast to const char* is needed as sendto expects a const void* for the message
    const char* buffer = reinterpret_cast<const char*>(message.data());

    if (their_addr == nullptr) {
        std::cerr << "ERR: Destination address is null." << std::endl;
        return false;
    }

    ssize_t bytes = sendto(server_socket, buffer, len, 0, (struct sockaddr *)their_addr, sizeof(*their_addr));
    TimedMessage timed_msg{message, std::chrono::steady_clock::now(), 0};

    if (bytes < 0) {
        std::cerr << "ERR: UDP Send failed: " << strerror(errno) << std::endl;
        return false; // Send failed
    }

    if (!waiting_for_confirm) {
        waiting_for_confirm = true;
        pending = timed_msg;
    }

    return true; // Message sent successfully
}

bool ChatClient::sendByeMessage() {
    ByeMessage bye_message;

    return tcp ? sendMessage(bye_message.serialize()) : sendMessage(bye_message.serialize(mid));
}

bool ChatClient::sendAuthMessage(const std::string& username, const std::string& display_name, const std::string& secret) {
    AuthMessage auth_message(username, display_name, secret);
    return tcp ? sendMessage(auth_message.serialize()) : sendMessage(auth_message.serialize(mid));
}

bool ChatClient::sendJoinMessage(const std::string& channelID, const std::string& display_name) {
    JoinMessage join_message(channelID, display_name);
    return tcp ? sendMessage(join_message.serialize()) : sendMessage(join_message.serialize(mid));
}

bool ChatClient::sendMsgMessage(const std::string& display_name, const std::string& content) {
    MsgMessage msg_message(display_name, content);
    return tcp ? sendMessage(msg_message.serialize()) : sendMessage(msg_message.serialize(mid));
}

bool ChatClient::sendConfirmMessage(const uint16_t message_id) {
    ConfirmMessage confirm_message(message_id);

    auto message = confirm_message.serialize();
    ssize_t len = message.size();

    // Cast to const char* is needed as sendto expects a const void* for the message
    const char* buffer = reinterpret_cast<const char*>(message.data());
    ssize_t bytes = sendto(server_socket, buffer, len, 0, (struct sockaddr *)their_addr, sizeof(*their_addr));

    if (bytes < 0) {
        std::cerr << "ERR: UDP Confirm Send failed" << std::endl;
        return false; // Send failed
    }

    return true; // Message sent successfully
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
        if (bye || err) return;

        const int timeout_duration = waiting_for_confirm ? 50 : -1; // Wait for 250ms if waiting for confirmation, else wait indefinitely

        int ret = poll(fds, 2, timeout_duration);  // Wait indefinitely until there's activity
        if (ret == -1) {
            // Handle error
            std::cerr << "ERR: poll" << std::endl;
            break;
        } else if (ret == 0) {
            // Timeout occurred
            if (waiting_for_confirm && !err) {
                checkForTimeouts();
            }
            continue;
        }

        // Check for incoming messages from the server
        if (fds[0].revents & POLLIN) {
            receiveMessage();
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
                } else if (command == "/auth" || !waiting_for_auth) {
                    if (command_queue.size() == 0) {
                        processCommand(input);
                    } else {
                        command_queue.push(input);
                    }
                } else if (waiting_for_response || waiting_for_confirm) {
                    std::cerr << "ERR: waiting for response(" << waiting_for_response << ")/confirm(" << waiting_for_confirm << ") from server" << std::endl;

                    // Queue other commands when waiting for a response
                    command_queue.push(input);
                } else {
                    std::cerr << "ERR: you must authenticate first" << std::endl;
                    printHelp();
                }
            } else if (std::cin.eof()) {
                std::cerr << "EOF detected on stdin. Shutting down..." << std::endl;
                return;
            } else {
                std::cerr << "Error reading from stdin. Shutting down..." << std::endl;
                return;
            }
        }

        // If not waiting for a response and there are queued commands, process the next one
        while (!waiting_for_response && command_queue.size() != 0) {
            std::cerr << "QUEUE" << std::endl;
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
            if (waiting_for_auth) {
                if (sendAuthMessage(params[0], params[1], params[2])) {
                    username = params[0];
                    secret = params[1];
                    display_name = params[2];
                    waiting_for_response = true;
                }
            } else {
                std::cerr << "ERR: Trying to send multiple /auth" << std::endl;
            }
        } else if (command == "/join" && params.size() == 1 && isValidId(params[0])) {
            if (sendJoinMessage(params[0], display_name)) {
                waiting_for_response = true;
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
        }
}

void ChatClient::processMessage(const std::string& message) {
    MessageType type = determineMessageType(message);
    switch (type) {
        case MessageType::ERR:
            {
                ErrorMessage msg = ErrorMessage::deserialize(message);

                std::cerr << "ERR FROM " << msg.display_name << ": " << msg.message_content << std::endl;

                err = true;

                break;
            }
        case MessageType::BYE:
            bye = true;
            std::cerr << "ERR: Received BYE message. Exiting..." << std::endl;
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

                if (msg.success && waiting_for_auth) waiting_for_auth = false;

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

void ChatClient::processMessage(const std::vector<uint8_t>& message) {
    MessageType type = determineMessageType(message);
    switch (type) {
        case MessageType::ERR:
            {
                ErrorMessage msg = ErrorMessage::deserialize(message);

                if (!sendConfirmMessage(msg.mid)) std::cerr << "ERR: confirm message is not sent" << std::endl;

                std::cerr << "ERR FROM " << msg.display_name << ": " << msg.message_content << std::endl;

                err = true;

                break;
            }
        case MessageType::BYE:
            bye = true;
            std::cerr << "ERR: Received BYE message. Exiting..." << std::endl;
            break;
        case MessageType::MSG:
            {
                MsgMessage msg = MsgMessage::deserialize(message);

                if (!sendConfirmMessage(msg.mid)) std::cerr << "ERR: confirm message is not sent" << std::endl;

                std::cout << msg.display_name << ": " << msg.message_content << std::endl;

                break;
            }
        case MessageType::REPLY:
            {
                ReplyMessage msg = ReplyMessage::deserialize(message);

                if (!sendConfirmMessage(msg.mid)) std::cerr << "ERR: confirm message is not sent" << std::endl;

                if (msg.success && waiting_for_auth) waiting_for_auth = false;

                std::cerr << (msg.success ? "Success: " : "Failure: ") << msg.message_content << std::endl;

                waiting_for_response = false;

                break;
            }
        case MessageType::CONFIRM:
            {
                ConfirmMessage msg = ConfirmMessage::deserialize(message);

                if (msg.message_id == mid) {
                    if (!checkForTimeouts()) {
                        pending = TimedMessage();
                        mid++;
                        waiting_for_confirm = false;
                    } else {
                        
                    }
                } else {
                    std::cerr << "ERR: caught CONFIRM with wrong message ID" << std::endl;
                }

                break;
            }
        default:
            std::cerr << "ERR: Unknown or malformed UDP message received." << std::endl;
            break;
    }
}

void ChatClient::receiveMessage() {
    if (tcp) {
        char buffer[1500] = {};
        ssize_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);

        std::string message;
        if (bytes_received > 0) message.assign(buffer, bytes_received);

        if (!message.empty()) processMessage(message);
    } else {
        std::vector<uint8_t> buffer(1500);
        struct sockaddr_in sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        ssize_t bytes_received = recvfrom(server_socket, buffer.data(), buffer.size(), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);

        if (bytes_received > 0) {
            their_addr->sin_family = sender_addr.sin_family;
            their_addr->sin_port = sender_addr.sin_port;
            their_addr->sin_addr = sender_addr.sin_addr;

            processMessage(std::vector<uint8_t>(buffer.begin(), buffer.begin() + bytes_received));
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
    if (!bye) sendByeMessage();

    // if (!tcp && waiting_for_confirm && !err) {
    //     receiveMessage();
    // }
}

bool ChatClient::checkForTimeouts() {
    auto now = std::chrono::steady_clock::now();
    
    if (!pending.message_data.empty()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - pending.send_time) > TIMEOUT) {
            if (pending.retry_count < MAX_RETRIES) {
                sendMessage(pending.message_data);
                pending.send_time = now;
                pending.retry_count++;
                std::cerr << "ERR: Timeout, retransmitting. Message ID: " << mid << std::endl;
                return true;
            } else {
                err = true;
                std::cerr << "ERR: Max retry count reached" << std::endl;
                // exit here
            }
        }
    }
    return false;
}