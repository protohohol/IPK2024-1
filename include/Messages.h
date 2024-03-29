// Messages.h
#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <sstream>
#include <vector>
#include <chrono>

// AUTH message structure
struct AuthMessage {
    std::string username;
    std::string display_name;
    std::string secret;

    AuthMessage(const std::string& u, const std::string& d, const std::string& s)
    : username(u), display_name(d), secret(s) {}

    // Serialize the AUTH message to a string
    std::string serialize() const {
        std::ostringstream ss;
        
        ss << "AUTH " << username << " AS " << secret << " USING " << display_name << "\r\n";
        return ss.str();
    }

    // Serialize for UDP
    std::vector<uint8_t> serialize(uint16_t message_id) const {
        std::vector<uint8_t> message;
        message.push_back(0x02); // Message type for AUTH
        
        message.push_back((message_id >> 8) & 0xFF);
        message.push_back(message_id & 0xFF);

        // Add Username followed by null terminator
        message.insert(message.end(), username.begin(), username.end());
        message.push_back(0);

        // Add Secret followed by null terminator
        message.insert(message.end(), secret.begin(), secret.end());
        message.push_back(0);

        // Add DisplayName followed by null terminator
        message.insert(message.end(), display_name.begin(), display_name.end());
        message.push_back(0);

        return message;
    }
};

// JOIN message structure
struct JoinMessage {
    std::string channel_id;
    std::string display_name;

    JoinMessage(const std::string& cid, const std::string& d)
    : channel_id(cid), display_name(d) {}

    // Serialize the JOIN message to a string
    std::string serialize() const {
        std::ostringstream ss;

        ss << "JOIN " << channel_id << " AS " << display_name << "\r\n";
        return ss.str();
    }

    // Serialize for UDP
    std::vector<uint8_t> serialize(uint16_t message_id) const {
        std::vector<uint8_t> message;
        message.push_back(0x03); // Assuming 0x03 is the message type for JOIN
        
        message.push_back((message_id >> 8) & 0xFF);
        message.push_back(message_id & 0xFF);

        // Add ChannelID followed by null terminator
        message.insert(message.end(), channel_id.begin(), channel_id.end());
        message.push_back(0);

        // Add DisplayName followed by null terminator
        message.insert(message.end(), display_name.begin(), display_name.end());
        message.push_back(0);

        return message;
    }
};

// MSG message structure
struct MsgMessage {
    std::string display_name;
    std::string message_content;

    uint16_t mid;
    
    MsgMessage() {}

    explicit MsgMessage(const std::string& d, const std::string& c)
    : display_name(d), message_content(c) {}

    // Serialize the MSG message to a string
    std::string serialize() const {
        std::ostringstream ss;

        ss << "MSG FROM " << display_name << " IS " << message_content << "\r\n";
        return ss.str();
    }

    // Serialize for UDP
    std::vector<uint8_t> serialize(uint16_t message_id) const {
        std::vector<uint8_t> message;
        message.push_back(0x04); // Assuming 0x04 is the message type for MSG

        message.push_back((message_id >> 8) & 0xFF);
        message.push_back(message_id & 0xFF);

        // Display Name and Message Content
        message.insert(message.end(), display_name.begin(), display_name.end());
        message.push_back(0);
        message.insert(message.end(), message_content.begin(), message_content.end());
        message.push_back(0);

        return message;
    }

    // Deserialize a string to a MsgMessage
    static MsgMessage deserialize(const std::string& str) {
        std::istringstream ss(str);
        MsgMessage msg;
        std::string type, from, is;
        
        // Extracting the type, 'FROM', and 'IS' tokens and discarding them
        ss >> type >> from >> msg.display_name >> is;
        
        // Using std::noskipws to include leading whitespaces in the content
        std::string content;
        std::getline(ss >> std::ws, content); // The rest of the stream is the message content
        
        // Removing the trailing "\r\n" if present
        if (!content.empty() && content.back() == '\r') {
            content.pop_back();
        }
        
        msg.message_content = content;
        return msg;
    }

    // Deserialize from UDP
    static MsgMessage deserialize(const std::vector<uint8_t>& data) {
        size_t index = 1;
        
        MsgMessage msg;
        msg.mid = static_cast<uint16_t>(data[index] << 8 | data[index + 1]);
        index += 2; // Move index past the message_id

        while (data[index] != 0) { // Extract display_name
            msg.display_name += static_cast<char>(data[index++]);
        }
        index++; // Skip null terminator

        while (data[index] != 0) { // Extract message_content
            msg.message_content += static_cast<char>(data[index++]);
        }
        // No need to skip null terminator at the end

        return msg;
    }
};

// REPLY message structure
struct ReplyMessage {
    bool success;
    std::string message_content;

    uint16_t mid;

    static ReplyMessage deserialize(const std::string& str) {
        std::istringstream ss(str);
        ReplyMessage msg;
        std::string type, success, is;

        ss >> type >> success; // Skip the type and read success as integer
        msg.success = (success == "OK");

        ss >> is; // Skip IS

        std::string content;
        std::getline(ss >> std::ws, content); // The rest of the stream is the message content
        
        // Removing the trailing "\r\n" if present
        if (!content.empty() && content.back() == '\r') {
            content.pop_back();
        }
        
        msg.message_content = content;
        return msg;
    }

    // UDP Deserialization
    static ReplyMessage deserialize(const std::vector<uint8_t>& data) {
        size_t index = 1;
        
        ReplyMessage msg;
        msg.mid = static_cast<uint16_t>(data[index] << 8 | data[index + 1]);
        index += 2;
        
        msg.success = data[index++] != 0; // Success byte (1 for true, 0 for false)

        index += 2;

        // The rest is message content
        while (index < data.size() && data[index] != 0) {
            msg.message_content += static_cast<char>(data[index++]);
        }

        return msg;
    }
};

// ERR message structure
struct ErrorMessage {
    std::string display_name;
    std::string message_content;

    uint16_t mid;

    ErrorMessage() {}

    explicit ErrorMessage(const std::string& d, const std::string& c)
    : display_name(d), message_content(c) {}

    std::string serialize() const {
        std::ostringstream ss;
        ss << "ERR FROM " << display_name << " IS " << message_content << "\r\n";
        return ss.str();
    }

    static ErrorMessage deserialize(const std::string& str) {
        std::istringstream ss(str);
        ErrorMessage msg;
        std::string type, from, is;

        ss >> type >> from >> std::ws; // Skip the "ERR FROM"
        std::getline(ss, msg.display_name, ' '); // Extract display_name up to the next space
        ss >> is; // Skip the "IS"
        
        std::string content;
        std::getline(ss >> std::ws, content); // The rest of the stream is the message content
        
        // Removing the trailing "\r\n" if present
        if (!content.empty() && content.back() == '\r') {
            content.pop_back();
        }
        
        msg.message_content = content;
        return msg;
    }

    // UDP Serialization
    std::vector<uint8_t> serialize(uint16_t message_id) const {
        std::vector<uint8_t> message;
        message.push_back(0xFE);
        message.push_back((message_id >> 8) & 0xFF);
        message.push_back(message_id & 0xFF);

        for (auto &ch : display_name) {
            message.push_back(static_cast<uint8_t>(ch));
        }
        message.push_back(0); // Null terminator for display_name

        for (auto &ch : message_content) {
            message.push_back(static_cast<uint8_t>(ch));
        }
        message.push_back(0); // Null terminator for message_content

        return message;
    }

    // UDP Deserialization
    static ErrorMessage deserialize(const std::vector<uint8_t>& data) {
        size_t index = 1;
        
        ErrorMessage msg;
        msg.mid = static_cast<uint16_t>(data[index] << 8 | data[index + 1]);
        index += 2;

        // Extracting display_name
        while (index < data.size() && data[index] != 0) {
            msg.display_name += static_cast<char>(data[index++]);
        }
        index++; // Skipping null terminator for display_name

        // Extracting message_content
        while (index < data.size() && data[index] != 0) {
            msg.message_content += static_cast<char>(data[index++]);
        }

        return msg;
    }
};

// BYE message structure
struct ByeMessage {
    std::string serialize() const {
        return "BYE\r\n";
    }

    // UDP Serialization
    std::vector<uint8_t> serialize(uint16_t message_id) const {
        std::vector<uint8_t> message;
        
        message.push_back(0xFF);
        message.push_back(message_id >> 8);
        message.push_back(message_id & 0xFF);

        // Since BYE messages have no content beyond the type and ID, no additional data is appended
        return message;
    }
};

struct ConfirmMessage {
    uint16_t message_id;

    ConfirmMessage() {}

    explicit ConfirmMessage(uint16_t id) : message_id(id) {}

    // Serialize the CONFIRM message to a binary format for UDP
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> message;

        message.push_back(0x00); // Assuming 0x00 represents a CONFIRM message type
        message.push_back(message_id >> 8);
        message.push_back(message_id & 0xFF);

        return message;
    }

    // Deserialize a binary data array to a ConfirmMessage
    static ConfirmMessage deserialize(const std::vector<uint8_t>& data) {
        ConfirmMessage msg;
        msg.message_id = (static_cast<uint16_t>(data[1]) << 8) | static_cast<uint16_t>(data[2]);

        return msg;
    }
};

struct TimedMessage {
    std::vector<uint8_t> message_data; // Message data
    std::chrono::steady_clock::time_point send_time; // Time when the message was last sent
    int retry_count = 0; // Number of times the message has been retried
};

#endif // MESSAGES_H