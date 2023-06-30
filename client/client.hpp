#pragma once

#include <iostream>

#include <string>
#include <stdint.h>
#include <sstream>

#if defined(__APPLE__)
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
#define CLOSE_SOCKET(sockfd) close(sockfd)
#define PLATFORM_INIT() \
    [] {                \
        return true;    \
    }()
#define PLATFORM_CLEANUP() \
    [] {                   \
        return true;       \
    }()
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
#define CLOSE_SOCKET(sockfd) closesocket(sockfd)
#define PLATFORM_INIT()                                 \
    [] {                                                \
        WSAData wsaData;                                \
        memset(&wsaData, 0, sizeof(wsaData));           \
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData); \
        return res == 0;                                \
    }()

#define PLATFORM_CLEANUP()        \
    [] {                          \
        return WSACleanup() == 0; \
    }()
#endif

#include "../shared/log.hpp"
#include "../shared/headers.hpp"
#include "../shared/parser_url.hpp"
#include "../shared/ip.hpp"

class Client
{
  private:

    std::string m_url;
    URI         m_uri;

  public:
    Client(const std::string& url);
    void setup();

  private:
    void getRequestIP(); // Function to send/accept data
    bool sendRequest(socket_t sockfd) const;
    bool readData(socket_t sockfd, char* buffer, size_t buffer_size) const;
};
