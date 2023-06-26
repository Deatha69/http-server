#include "client.h"
#include "../additional/ip.h"

Client::Client(const std::string& url) : m_url(url) { }

void Client::setup()
{
    constexpr size_t httpPrefixSize = 7;
    if (m_url.substr(0, httpPrefixSize) != "http://") {
        parseIP();
    } else {
        parseURL();
    }
    getRequestIP();
}

void Client::parseIP()
{
    size_t pos = m_url.find(":");
    if (pos == std::string::npos) {
        IP ip(m_url, "80");
        m_uri.host = ip.getAddress();
        m_uri.port = ip.getPort();
        m_uri.path = "/";
    } else {
        IP ip(m_url.substr(0, pos), m_url.substr(pos + 1));
        m_uri.host = ip.getAddress();
        m_uri.port = ip.getPort();
        m_uri.path = "/";
    }
}

void Client::parseURL()
{
    m_url                   = m_url.substr(7);
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
    struct hostent*  he;
    struct in_addr** addr_list;

    if ((he = gethostbyname(m_uri.host.c_str())) == NULL) {
        LOG_ERROR("Failed to resolve comain {} ip!", m_uri.host);
        throw std::runtime_error("Failed convert domain \"" + m_uri.host + "\" to ip");
    }

    addr_list = (struct in_addr**)he->h_addr_list;
    IP ip(inet_ntoa(*addr_list[0]), "80");
    m_uri.host = ip.getAddress();
}

void Client::getRequestIP()
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
    if ((n = getaddrinfo(m_uri.host.c_str(), m_uri.port.c_str(), &hints, &res)) != 0) {
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
    request += "Host: " + m_uri.host + "\r\n";
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
