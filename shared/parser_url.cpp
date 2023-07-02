#include "parser_url.hpp"

void Parser::parseURL(const std::string& url, URI& uri)
{
    std::string  parsedURL  = url.substr(7);
    const size_t slashIndex = parsedURL.find_first_of('/');
    if (slashIndex == std::string::npos) {
        uri.host = parsedURL;
        uri.path = "/";
    } else {
        uri.host = parsedURL.substr(0, slashIndex);
        uri.path = parsedURL.substr(slashIndex);
    }

    const size_t colonIndex = uri.host.find_first_of(':');
    if (colonIndex != std::string::npos) {
        uri.port = uri.host.substr(colonIndex + 1);
        uri.host = uri.host.substr(0, colonIndex);
    } else {
        uri.port = "80";
    }

    struct hostent*  he;
    struct in_addr** addr_list;

    if ((he = gethostbyname(uri.host.c_str())) == NULL) {
        LOG_ERROR("Failed to resolve domain {} ip!", uri.host);
        throw std::runtime_error("Failed convert domain \"" + uri.host + "\" to IP");
    }

    addr_list = (struct in_addr**)he->h_addr_list;
    IP ip(inet_ntoa(*addr_list[0]), "80");
    uri.host = ip.getAddress();
}
