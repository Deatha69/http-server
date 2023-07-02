#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>

#include "../shared/headers.hpp"
#include "../shared/log.hpp"
#include "../shared/parser_http.hpp"

class Server
{
  private:
    int                m_server_socket;
    struct sockaddr_in m_server_addr;

    std::vector<std::string> m_startLine;
    Headers                  m_headers;
    std::string              m_data;

  public:
    Server(int port);
    ~Server();

    void start();

  private:
    int accept_connection();
    void handle_client(int client_socket);
    std::string read_request(int client_socket);
    std::string process_request(const std::string& request);
    void send_response(int client_socket, const std::string& response);
    void clear_request();
};

