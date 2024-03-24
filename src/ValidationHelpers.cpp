#include "ValidationHelpers.h"

bool isValidId(const std::string& id) {
    std::regex id_pattern(R"(^[A-Za-z0-9-]{1,20}$)");
    return std::regex_match(id, id_pattern);
}

bool isValidSecret(const std::string& secret) {
    std::regex secret_pattern(R"(^[A-Za-z0-9-]{1,128}$)");
    return std::regex_match(secret, secret_pattern);
}

bool isValidDName(const std::string& dname) {
    std::regex dname_pattern(R"(^[\x21-\x7E]{1,20}$)"); // VCHAR matches characters in the range 0x21-0x7E
    return std::regex_match(dname, dname_pattern);
}

bool isValidContent(const std::string& content) {
    std::regex content_pattern(R"(^[\x20-\x7E]{1,1400}$)"); // VCHAR plus space (0x20)
    return std::regex_match(content, content_pattern);
}