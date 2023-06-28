#include <unordered_map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <string_view>

#include "headers.hpp"

Headers& Headers::operator=(const Headers& other)
{
    if (this != &other)
        data = other.data;
    return *this;
}

Headers& Headers::operator=(Headers&& other) noexcept
{
    if (this != &other)
        data = std::move(other.data);
    return *this;
}

bool Headers::operator==(const Headers& other) const
{
    return data == other.data;
}

bool Headers::operator!=(const Headers& other) const
{
    return !(*this == other);
}

size_t Headers::size() const
{
    return data.size();
}

std::string Headers::to_string() const
{
    std::string out;

    // Calculate the total length of the string, to avoid relocations
    size_t total_size = 0;
    std::for_each(data.begin(), data.end(), [&](const auto& pair) {
        total_size += pair.first.length();  // reserve space for key
        total_size += 2;                    // reserve space for ": " between key and value
        total_size += pair.second.length(); // reserve space for value
        total_size += 2;                    // reserve space for \r\n
    });

    out.reserve(total_size);
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
        out += it->first;
        out += ": ";
        out += it->second;
        out += "\r\n";
    }
    return out;
}

std::string& Headers::operator[](const std::string& key)
{
    return data[key];
}

const std::string& Headers::operator[](const std::string& key) const
{
    auto it = data.find(key);
    if (it == data.cend()) {
        throw std::out_of_range("Header \"" + key + "\" does not exist");
    }
    return it->second;
}

std::string& Headers::at(const std::string& key)
{
    return data.at(key);
}

const std::string& Headers::at(const std::string& key) const
{
    return data.at(key);
}

void Headers::from_string(const std::string& str)
{
    data.clear();

    std::istringstream iss(str);

    std::string line;
    line.reserve(str.size());

    while (std::getline(iss, line) && !line.empty()) {
        auto separatorIndex = line.find(':');
        if (separatorIndex == std::string::npos) {
            throw std::invalid_argument("Invalid headers format");
        }

        std::string key   = line.substr(0, separatorIndex);
        std::string value = line.substr(separatorIndex + 1);

        key.erase(key.begin(), std::find_if(key.begin(), key.end(), [](unsigned char ch) {
                      return !std::isspace(ch);
                  }));
        key.erase(std::find_if(key.rbegin(), key.rend(),
                               [](unsigned char ch) {
                                   return !std::isspace(ch);
                               })
                      .base(),
                  key.end());

        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
                        return !std::isspace(ch);
                    }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
                                 [](unsigned char ch) {
                                     return !std::isspace(ch);
                                 })
                        .base(),
                    value.end());

        bool isKeyValid = std::all_of(key.begin(), key.end(), [](char c) {
            return std::isalnum(c) || c == '-';
        });
        if (!isKeyValid) {
            throw std::invalid_argument("Invalid character in header key: " + key);
        }

        data.emplace(std::move(key), std::move(value));
    }
}