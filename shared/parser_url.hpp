#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include "../shared/log.hpp"

struct URI
{
    std::string host;
    std::string port;
    std::string path;
};

class Parser
{
  public:
    static void parseURL(const std::string& url, URI& uri);
};
