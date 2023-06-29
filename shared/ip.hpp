#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include "log.hpp"

class IP
{
  private:
    std::string m_address;
    std::string m_port;

  public:
    IP(const std::string& address, const std::string& port);

    
    static bool isPortValid(const std::string& port); // Function for checking the port
    static bool isIpAddressValid(const std::string& ip_address); // Function for checking the ip address
    const std::string& getAddress() const;
    const std::string& getPort() const;
};