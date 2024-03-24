#ifndef VALIDATIONHELPERS_H
#define VALIDATIONHELPERS_H

#include <string>
#include <regex>

bool isValidId(const std::string& id);
bool isValidSecret(const std::string& secret);
bool isValidDName(const std::string& dname);
bool isValidContent(const std::string& content);

#endif // VALIDATIONHELPERS_H