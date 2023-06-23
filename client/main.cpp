
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

class IP
{
  private:
    std::string m_address;
    std::string m_port;

  public:
    IP() = default;
    IP(const std::string& address, const std::string& port = "80") : m_address{address}, m_port{port}
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
    static bool isPortValid(const std::string& port)
    {
        int portNumber = std::stoi(port);
        return (portNumber >= 0 && portNumber <= 65535);
    }

    // Function for checking the ip address
    static bool isIpAddressValid(const std::string& ip_address)
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

    const std::string& getAddress() const
    {
        return m_address;
    }

    const std::string& getPort() const
    {
        return m_port;
    }
};

class Client
{
  private:
    struct URI
    {
        std::string ip;
        std::string host;
        std::string port;
        std::string path;
    };

    std::string m_url;
    URI         m_uri;
    IP          ip;

  public:
    Client(const std::string& url) : m_url(url)
    {
        parseData();
    }

    void getRequestIP()
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
        if ((n = getaddrinfo(m_uri.ip.c_str(), m_uri.port.c_str(), &hints, &res)) != 0) {
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
        request = "GET " + m_uri.path + " HTTP/1.0\r\n";
        request += "Host: " + m_uri.ip + "\r\n";
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

  private:
    // Function to parse data
    void parseData()
    {
        constexpr size_t httpPrefixSize = 7;
        if (m_url.substr(0, httpPrefixSize) != "http://") {
            size_t pos = m_url.find(":");
            if (pos == std::string::npos) {
                IP ip(m_url, "80");
                m_uri.ip = ip.getAddress();
                m_uri.port = ip.getPort();
                m_uri.path = "/";
            } else {
                IP ip(m_url.substr(0, pos), m_url.substr(pos + 1));
                m_uri.ip = ip.getAddress();
                m_uri.port = ip.getPort();
                m_uri.path = "/";
            }
        } else {
            m_url                   = m_url.substr(httpPrefixSize);
            const size_t slashIndex = m_url.find_first_of('/');
            if (slashIndex == std::string::npos) {
                m_uri.host = m_url;
                m_uri.path = "/";
            } else {
                m_uri.host = m_url.substr(0, slashIndex);
                m_uri.path = m_url.substr(slashIndex);
            }

            const size_t colonIndex = m_uri.host.find_first_of(':');
            if (colonIndex != std::string::npos) {
                m_uri.port = m_uri.host.substr(colonIndex + 1);
                m_uri.host = m_uri.host.substr(0, colonIndex);

            } else {
                m_uri.port = "80";
            }
            domainToIp();
        }
    }
    // Function to convert domain name to ip
    void domainToIp()
    {
        struct hostent*  he;
        struct in_addr** addr_list;
        std::string      result;

        if ((he = gethostbyname(m_uri.host.c_str())) == NULL) {
            LOG_ERROR("Failed to resolve comain {} ip!", m_uri.host);
            throw std::runtime_error("Failed convert domain \"" + m_uri.host + "\" to ip");
        }

        addr_list = (struct in_addr**)he->h_addr_list;
        IP ip(inet_ntoa(*addr_list[0]));
        m_uri.ip = ip.getAddress();
    }
};

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

    try {
        Client client(argv[1]);
        client.getRequestIP();
    } catch (const std::exception& e) {
        LOG_ERROR("Exception has occurred: {}", e.what());
    }

    PLATFORM_CLEANUP();

    return 0;
}