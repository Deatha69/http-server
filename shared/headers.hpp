#pragma once
#ifndef GUARD_HEADERS_HPP
#define GUARD_HEADERS_HPP

#include <unordered_map>
#include <string>
#include <algorithm>
#include <string_view>

struct Headers
{
    std::unordered_map<std::string, std::string> data;

    Headers() = default;
    Headers(const std::initializer_list<std::pair<const std::string, std::string>>& list) : data(list) { }
    Headers(const Headers& other) : data(other.data) { }
    Headers(Headers&& other) noexcept : data(std::move(other.data)) { }

    Headers& operator=(const Headers& other);
    Headers& operator=(Headers&& other) noexcept;

    std::string&       operator[](const std::string& key);
    const std::string& operator[](const std::string& key) const;
    std::string&       at(const std::string& key);
    const std::string& at(const std::string& key) const;

    bool operator==(const Headers& other) const;
    bool operator!=(const Headers& other) const;

    size_t size() const;

    std::string to_string() const;
    void        from_string(const std::string& str);
};

#endif