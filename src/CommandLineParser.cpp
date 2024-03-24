#include "CommandLineParser.h"
#include <cstring>

void CommandLineParser::printUsage() {
    std::cerr << "usage: ipk24chat-client -s <server_ip_or_hostname> -p <port> -t <tcp_or_udp> [-d <udp_confirmation_timeout>] [-r <udp_retransmissions>]\n";
    std::cout << "  -t tcp|udp\tTransport protocol used for connection (required).\n";
    std::cout << "  -s <host>\tServer IP address or hostname (required).\n";
    std::cout << "  -p <port>\tServer port, default is 4567 (optional).\n";
    std::cout << "  -d <timeout>\tUDP confirmation timeout in milliseconds, default is 250 (optional).\n";
    std::cout << "  -r <retransmissions>\tMaximum number of UDP retransmissions, default is 3 (optional).\n";
    std::cout << "  -h\t\tPrints this help output and exits.\n";
}

void CommandLineParser::printConfig(AppConfig config) {
    std::cout << "Server Address: " << config.server_address << ", Port: " << config.port << std::endl;

    std::cout << "\nTransport protocol:\t" << config.transport_protocol
              << "\nServer IP or hostname:\t" << config.server_address
              << "\nServer port:\t" << config.port
              << "\nUDP confirmation timeout:\t" << config.timeout
              << "\nMaximum number of UDP retransmissions:\t" << static_cast<int>(config.retransmissions_number)
              << std::endl;
}

AppConfig CommandLineParser::parseArguments(int argc, char* argv[]) {
    AppConfig config;

    std::string server_address, transport_protocol;

    int opt;

    while((opt = getopt(argc, argv, "t:s:p:d:r:h")) != -1) {
        switch(opt) {
            case 't':
                if (strcmp(optarg, "udp") || strcmp(optarg, "tcp")) {
                    config.transport_protocol = optarg;
                } else {
                    std::cerr << "Wrong transport protocol name!" << std::endl;
                    config.valid = false;
                    return config;
                }
                break;
            case 's':
                config.server_address = optarg;
                break;
            case 'p':
                try {
                    config.port = std::stoi(optarg);
                } catch (const std::out_of_range& e) {
                    std::cerr << "Wrong port number : " << e.what() << std::endl;
                    config.valid = false;
                    return config;
                }
                break;
            case 'd':
                try {
                    config.timeout = std::stoi(optarg);
                } catch (const std::out_of_range& e) {
                    std::cerr << "Wrong timeout number : " << e.what() << std::endl;
                    config.valid = false;
                    return config;
                }
                break;
            case 'r':
                try {
                    config.retransmissions_number = std::stoi(optarg);
                } catch (const std::out_of_range& e) {
                    std::cerr << "Wrong retransmissions number : " << e.what() << std::endl;
                    config.valid = false;
                    return config;
                }
                break;
            case 'h':
                config.show_help = true;
                break;
            case '?':
            default:
                config.valid = false;
                return config;
        }
    }

    return config;
}