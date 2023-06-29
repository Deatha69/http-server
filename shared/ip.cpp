#include "ip.hpp"

IP::IP(const std::string& address, const std::string& port) : m_address{address}, m_port{port}
{
    if (!isIpAddressValid(address)) {
        LOG_ERROR("Invalid ip!", address);
        throw std::invalid_argument("Invalid IP address format");
    }
    if (!isPortValid(port)) {
        LOG_ERROR("Invalid port!", port);
        throw std::invalid_argument("Invalid port number");
    }
}

// Function for checking the port
bool IP::isPortValid(const std::string& port)
{
    int portNumber = std::stoi(port);
    return (portNumber >= 0 && portNumber <= 65535);
}

// Function for checking the ip address
bool IP::isIpAddressValid(const std::string& ip_address)
{
    std::vector<std::string> parts;
    std::stringstream        ss(ip_address);
    std::string              part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    if (parts.size() != 4) {
        return false;
    }

    for (const auto& part : parts) {
        try {
            int num = std::stoi(part);
            if (num < 0 || num > 255) {
                return false;
            }
        } catch (std::invalid_argument&) {
            return false;
        }
    }

    return true;
}

const std::string& IP::getAddress() const
{
    return m_address;
}

const std::string& IP::getPort() const
{
    return m_port;
}
