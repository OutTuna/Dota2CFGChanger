#pragma once
#include <string>

const char* tr(const char* key);
std::string tr_value(const char* key, const std::string& value);
void set_language(const std::string& code);
const char* language_code();
int language_index();
