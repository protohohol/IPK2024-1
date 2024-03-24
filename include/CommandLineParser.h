#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include "AppConfig.h"
#include <unistd.h>
#include <iostream>
#include <optional>

class CommandLineParser {
public:
    static AppConfig parseArguments(int argc, char* argv[]);
    static void printUsage();
    static void printConfig(AppConfig config);
};

#endif // COMMANDLINEPARSER_H