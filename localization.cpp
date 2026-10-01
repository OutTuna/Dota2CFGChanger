#include "localization.h"
#include "embedded_locales.h"
#include <nlohmann/json.hpp>
#include <array>
#include <atomic>
#include <map>

namespace {
std::atomic<int> current_language{0};
const char* codes[] = {"en", "ru", "uk"};
const auto& dictionaries() {
    static const auto values = [] {
        std::array<std::map<std::string, std::string>, 3> result;
        for (int i = 0; i < 3; ++i)
            result[i] = nlohmann::json::parse(embedded_locales::json[i]).get<std::map<std::string, std::string>>();
        return result;
    }();
    return values;
}
}

const char* tr(const char* key) {
    const auto& values = dictionaries();
    const auto& selected = values[current_language.load()];
    auto found = selected.find(key);
    if (found != selected.end()) return found->second.c_str();
    auto fallback = values[0].find(key);
    return fallback != values[0].end() ? fallback->second.c_str() : key;
}

std::string tr_value(const char* key, const std::string& value) {
    std::string result = tr(key);
    auto pos = result.find("{value}");
    if (pos != std::string::npos) result.replace(pos, 7, value);
    return result;
}

void set_language(const std::string& code) {
    int index = 0;
    for (int i = 0; i < 3; ++i) if (code == codes[i]) index = i;
    current_language.store(index);
}

const char* language_code() { return codes[current_language.load()]; }
int language_index() { return current_language.load(); }
