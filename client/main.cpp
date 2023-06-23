
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

#include "log.hpp"

struct IP
{
    std::string address;
    std::string port;
};

struct URI
{
    std::string host;
    std::string port;
    std::string path;
    IP          ip;
};

// Function for checking the ip address
bool isIpAddressValid(const std::string& ip_address)
{
    std::vector<std::string> parts;
    std::stringstream        ss(ip_address);
    std::string              part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    if (parts.size() != 4) {
        throw std::runtime_error("IP address should contain exactly 4 parts");
        return false;
    }

    for (const auto& part : parts) {
        try {
            int num = std::stoi(part);
            if (num < 0 || num > 255) {
                throw std::runtime_error("Number " + part + " is not in the range from 0 to 255");
                return false;
            }
        } catch (std::invalid_argument&) {
            throw std::runtime_error("Part " + part + " is not a number");
            return false;
        }
    }

    return true;
}

// Function to convert domain name to ip
void domainToIp(URI& uri)
{
    struct hostent*  he;
    struct in_addr** addr_list;
    std::string      result;

    if ((he = gethostbyname(uri.host.c_str())) == NULL) {
        LOG_ERROR("Failed to resolve comain {} ip!", uri.host);
        throw std::runtime_error("Failed convert domain \"" + uri.host + "\" to ip");
    }

    addr_list      = (struct in_addr**)he->h_addr_list;
    uri.ip.address = inet_ntoa(*addr_list[0]);
    if (isIpAddressValid(uri.ip.address)) {
        uri.ip.port = uri.port;
    } else {
        LOG_ERROR("Invalid ip!", uri.ip.address);
        throw std::runtime_error("Invalid ip \"" + uri.ip.address + "\".");
    }
}

// Function to parse data
void parseData(URI& parsedUri, std::string url)
{
    constexpr size_t httpPrefixSize = 7;
    if (url.substr(0, httpPrefixSize) != "http://") {
        size_t pos = url.find(":");
        if (pos == std::string::npos) {
            if (isIpAddressValid(url)) {
                parsedUri.ip.address = url;
                parsedUri.ip.port    = "80";
                parsedUri.path       = "/";
            } else
                throw std::runtime_error("Invalid ip \"" + url + "\".");
        } else {
            if (isIpAddressValid(url.substr(0, pos))) {
                int portNumber = std::stoi(url.substr(pos + 1));
                if (portNumber >= 0 && portNumber <= 65535) {
                    parsedUri.ip.address = url.substr(0, pos);
                    parsedUri.ip.port    = url.substr(pos + 1);
                    parsedUri.path       = "/";
                } else 
                    throw std::runtime_error("Invalid port \"" + url.substr(pos + 1) + "\".");
            } else 
                throw std::runtime_error("Invalid ip \"" + url.substr(0, pos) + "\".");
        }

    } else {
        url                     = url.substr(httpPrefixSize);
        const size_t slashIndex = url.find_first_of('/');
        if (slashIndex == std::string::npos) {
            parsedUri.host = url;
            parsedUri.path = "/";
        } else {
            parsedUri.host = url.substr(0, slashIndex);
            parsedUri.path = url.substr(slashIndex);
        }

        const size_t colonIndex = parsedUri.host.find_first_of(':');
        if (colonIndex != std::string::npos) {
            parsedUri.port = parsedUri.host.substr(colonIndex + 1);
            parsedUri.host = parsedUri.host.substr(0, colonIndex);
        } else {
            parsedUri.port = "80";
        }
        domainToIp(parsedUri);
    }
}

void getRequestIP(const URI& uri)
{
    struct addrinfo hints, *res;

    socket_t    sockfd = 0;
    int         n;
    char        buffer[2048];
    std::string request, header, response;

    // Get data about IP-address
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //  if ((n = getaddrinfo(ip.c_str(), "http", &hints, &res)) != 0)
    if ((n = getaddrinfo(uri.ip.address.c_str(), uri.ip.port.c_str(), &hints, &res)) != 0) {
        LOG_ERROR("getaddrinfo: {}", gai_strerror(n));
        return;
    }

    // Create socket and setup connection
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        LOG_ERROR("Failed to create a socket");
        return;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        LOG_ERROR("Failed to connect to the socket");
        return;
    }

    freeaddrinfo(res);

    // Send request to server
    request = "GET " + uri.path + " HTTP/1.0\r\n";
    request += "Host: " + uri.ip.address + "\r\n";
    request += "User-Agent: Http/1.0\r\n\r\n";

    LOG_DEBUG("Sending request with data:\n{}", request);
    send(sockfd, request.c_str(), request.size(), 0);

    // Read data from server and display it
    size_t bytes;
    bool   headerEnded   = false;
    size_t contentLength = 0;

    while ((bytes = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        if (!headerEnded) {
            header.append(buffer, bytes);
            size_t pos = header.find("\r\n\r\n");
            if (pos != std::string::npos) {
                headerEnded = true;
                std::string contentLengthStr("Content-Length:");
                size_t      contentLengthPos = header.find(contentLengthStr);
                if (contentLengthPos != std::string::npos) {
                    contentLength = std::stoi(header.substr(contentLengthPos + contentLengthStr.length()));
                }
                if (contentLength == 0) {
                    break;
                }
            }
        }
        response.append(buffer, bytes);
        contentLength -= bytes;
        if (contentLength == 0) {
            break;
        }
    }

    LOG_DEBUG("Content received: {}", response);

    CLOSE_SOCKET(sockfd);
}

int main(int argc, char* argv[])
{
    LOG_INIT();

    if (!PLATFORM_INIT()) {
        LOG_CRITICAL("Unable to initialize the platform networking");
        return -1;
    }

    if (argc < 2) {
        LOG_CRITICAL("Incorrect input. Example: {} <URL>", argv[0]);
        return 1;
    }

    URI uri;

    try {
        parseData(uri, argv[1]);
        getRequestIP(uri);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception has occurred: {}", e.what());
    }

    PLATFORM_CLEANUP();

    return 0;
}