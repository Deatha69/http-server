#include "server.hpp"

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