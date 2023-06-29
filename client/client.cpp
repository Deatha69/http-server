#include "client.hpp"

Client::Client(const std::string& url) : m_url(url) { }

void Client::setup()
{
    Parser::parse(m_url, m_uri);
    getRequestIP();
}

void Client::getRequestIP()
{
    struct addrinfo hints, *res;

    socket_t sockfd = 0;
    int      n;
    std::vector<char> buffer(2048);


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
    if (!sendRequest(sockfd)) {
        CLOSE_SOCKET(sockfd);
        return;
    }

    // Read data from server and display it
    if (!readData(sockfd, buffer.data(), sizeof(buffer))) {
        CLOSE_SOCKET(sockfd);
        return;
    }

    CLOSE_SOCKET(sockfd);
}

bool Client::sendRequest(socket_t sockfd) const
{
    std::string request = "GET " + m_uri.path + " HTTP/1.0\r\n";
    request += "Host: " + m_uri.host + "\r\n";
    request += "User-Agent: Http/1.0\r\n\r\n";

    LOG_DEBUG("Sending request with data:\n{}", request);

    if (send(sockfd, request.c_str(), request.size(), 0) == -1) {
        LOG_ERROR("Failed to send request");
        return false;
    }

    return true;
}

bool Client::readData(socket_t sockfd, char* buffer, size_t buffer_size) const
{
    size_t      bytes;
    bool        headerEnded   = false;
    size_t      contentLength = 0;
    std::string header, response;

    while ((bytes = recv(sockfd, buffer, buffer_size, 0)) > 0) {
        if (!headerEnded) {
            header.append(buffer, bytes);
            size_t      pos = header.find("\r\n\r\n");
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

    return true;
}
