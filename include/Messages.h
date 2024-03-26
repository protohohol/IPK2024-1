// Messages.h
#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <sstream>
#include <vector>

// AUTH message structure
struct AuthMessage {
    std::string username;
    std::string display_name;
    std::string secret;

    // Serialize the AUTH message to a string
    std::string serialize() const {
        std::ostringstream ss;
        
        ss << "AUTH " << username << " AS " << secret << " USING " << display_name << "\r\n";
        return ss.str();
    }
};

// JOIN message structure
struct JoinMessage {
    std::string channel_id;
    std::string display_name;

    // Serialize the JOIN message to a string
    std::string serialize() const {
        std::ostringstream ss;

        ss << "JOIN " << channel_id << " AS " << display_name << "\r\n";
        return ss.str();
    }
};

// MSG message structure
struct MsgMessage {
    std::string display_name;
    std::string message_content;

    // Serialize the MSG message to a string
    std::string serialize() const {
        std::ostringstream ss;

        ss << "MSG FROM " << display_name << " IS " << message_content << "\r\n";
        return ss.str();
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
};

// REPLY message structure
struct ReplyMessage {
    bool success;
    std::string message_content;

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
};

// ERR message structure
struct ErrorMessage {
    std::string display_name;
    std::string message_content;

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
};

// BYE message structure
struct ByeMessage {
    std::string serialize() const {
        return "BYE\r\n";
    }
};

#endif // MESSAGES_H