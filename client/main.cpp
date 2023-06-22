#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct IP {
    std::string address;
    std::string port;
};

struct URI
{
    std::string host;
    std::string port;
    std::string path;
    IP ip;
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

// Function to convert domain name to ip
void domainToIp(URI& uri) {
    struct hostent*  he;
    struct in_addr** addr_list;
    std::string      result;

    if ((he = gethostbyname(uri.host.c_str())) == NULL) {
        result = "Idk how to call this error";
    }

    addr_list = (struct in_addr**)he->h_addr_list;
    uri.ip.address    = inet_ntoa(*addr_list[0]);
    uri.ip.port = uri.port;
}

void getRequestIP(const URI& uri) {
    struct addrinfo hints, *res;
    int             sockfd, n;
    char            buffer[2048];
    std::string     request, header, response;

    // Get data about IP-address
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //  if ((n = getaddrinfo(ip.c_str(), "http", &hints, &res)) != 0)
    if ((n = getaddrinfo(uri.ip.address.c_str(), uri.ip.port.c_str(), &hints, &res)) != 0) {
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
    request = "GET "+ uri.path + " HTTP/1.0\r\n";
    request += "Host: " + uri.ip.address + "\r\n";
    request += "User-Agent: Http/1.0\r\n\r\n";
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
    domainToIp(uri);
    getRequestIP(uri);
    return 0;
}