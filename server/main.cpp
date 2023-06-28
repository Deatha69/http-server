#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

#include "../shared/headers.hpp"
#include "../additional/log.hpp"

class Server
{
  private:
    int                m_server_socket;
    struct sockaddr_in m_server_addr;

    struct StartLine
    {
        std::string request;
        std::string path;
        std::string version;
    };

    StartLine line;

  public:
    Server(int port)
    {
        m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_server_socket == -1) {
            LOG_ERROR("Could not create socket");
            throw std::runtime_error("Could not create socket");
        }

        m_server_addr.sin_family      = AF_INET;
        m_server_addr.sin_addr.s_addr = INADDR_ANY;
        m_server_addr.sin_port        = htons(port);

        if (bind(m_server_socket, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr)) < 0) {
            LOG_ERROR("Bind failed");
            throw std::runtime_error("Bind failed");
        }

        listen(m_server_socket, 3);
        std::cout << "Server started. Listening on port " << port << "..." << std::endl;
    }

    ~Server()
    {
        close(m_server_socket);
    }

    void start()
    {
        while (true) {
            int client_socket = accept_connection();
            handle_client(client_socket);
            close(client_socket);
        }
    }

  private:
    int accept_connection()
    {
        struct sockaddr_in client_addr;
        socklen_t          client_addr_len = sizeof(client_addr);
        int                client_socket   = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            LOG_ERROR("Accept failed");
            throw std::runtime_error("Accept failed");
        }
        return client_socket;
    }

    void handle_client(int client_socket)
    {
        std::string request  = read_request(client_socket);
        std::string response = process_request(request);
        send_response(client_socket, response);
    }

    std::string read_request(int client_socket)
    {
        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            LOG_ERROR("Error reading request");
            throw std::runtime_error("Error reading request");
        }
        return std::string(buffer);
    }
    /*
    void parse_http_start_line(const std::string& start_line)
    {
        std::vector<std::string> parts;
        std::size_t              start = 0;
        std::size_t              end   = start_line.find(" ");
        while (end != std::string::npos) {
            parts.push_back(start_line.substr(start, end - start));
            start = end + 1;
            end   = start_line.find(" ", start);
        }
        parts.push_back(start_line.substr(start));

        if (parts.size() != 3) {
            throw std::invalid_argument("Invalid start line format");
        }

        line.request  = parts[0];
        line.path    = parts[1];
        line.version = parts[2];
    }
    */
    std::string process_request(const std::string& request)
    {
        /*
        std::size_t              end_of_start_line = request.find("\r\n");
        std::string              start_line        = request.substr(0, end_of_start_line);
        parse_http_start_line(start_line);
        */

        std::string response = "Black big African Rhino";
        return response;
    }

    void send_response(int client_socket, const std::string& response)
    {
        size_t bytes_sent = send(client_socket, response.c_str(), response.length(), 0);
        if (bytes_sent < 0) {
            LOG_ERROR("Error sending response");
            throw std::runtime_error("Error sending response");
        }
    }
};

int main()
{
    try {
        Server server(80);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}