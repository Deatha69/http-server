#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <regex>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// Uri struct <scheme/host:port/path>
struct URI
{
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
};

// Function to parse uri
void parseURL(URI& parsedUri, std::string url) {
    constexpr size_t httpPrefixSize = 7;
    if (url.substr(0, httpPrefixSize) != "http://") {
        throw std::invalid_argument("Sorry, this code only supports HTTP URLs.");
    }

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
}

// Function to get data from url
void geturl(const URI& uri, const std::string& filename = "") {
    struct addrinfo hints, *res;
    int             sockfd, n;
    char            buffer[2048];
    std::string     request, header, response;

    // Get data about host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((n = getaddrinfo(uri.host.c_str(), uri.port.c_str(), &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(n) << std::endl;
        return;
    }

    // Create socket and setup connection
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("socket");
        return;
    }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        return;
    }
    freeaddrinfo(res);

    // Send request to server
    request = "GET " + uri.path + " HTTP/1.0\r\n";
    request += "Host: " + uri.host + "\r\n";
    request += "User-Agent: HttpRequest/1.0\r\n\r\n";
    send(sockfd, request.c_str(), request.size(), 0);

    // Read data from server and display it 
    ssize_t bytes;
    bool    headerEnded   = false;
    size_t  contentLength = 0;

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
    std::cout << response;
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Incorrect input: " << argv[0] << " <URL> " << std::endl;
        return 1;
    }
    URI uri;
    parseURL(uri, argv[1]);
    geturl(uri, (argc > 2) ? argv[2] : "");
    return 0;
}