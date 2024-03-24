#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <string>

struct AppConfig {
    std::string transport_protocol, server_address;
    unsigned short port;
    unsigned short timeout;
    unsigned char retransmissions_number;
    bool show_help;
    bool valid; // Add a flag to indicate if the config is valid

    AppConfig() 
        : port(4567), 
          timeout(250), 
          retransmissions_number(3), 
          show_help(false), 
          valid(true) 
    {}
};

#endif // APPCONFIG_H