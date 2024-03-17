#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <string>

struct AppConfig {
    std::string transport_protocol, server_address;
    unsigned short port = 4567;
    unsigned short timeout = 250;
    unsigned char retransmissions_number = 3;
    bool show_help = false;
    bool valid = true; // Add a flag to indicate if the config is valid

    AppConfig() {}
};

#endif // APPCONFIG_H