#pragma once
#include <nlohmann/json.hpp>
#include <cstdint>
#include <stdexcept>

inline std::uint64_t json_unsigned(const nlohmann::json& value, std::uint64_t maximum) {
    if (!value.is_number_integer() || (!value.is_number_unsigned() && value.get<std::int64_t>() < 0))
        throw std::runtime_error("Expected a non-negative integer");
    auto result = value.get<std::uint64_t>();
    if (result > maximum) throw std::runtime_error("Integer is out of range");
    return result;
}
