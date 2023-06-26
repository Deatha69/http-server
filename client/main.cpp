#include "client.h"

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
        client.setup();
    } catch (const std::exception& e) {
        LOG_ERROR("Exception has occurred: {}", e.what());
    }

    PLATFORM_CLEANUP();

    return 0;
}