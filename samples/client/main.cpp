#include <iostream>

#include <falcon_API.h>

#include "spdlog/spdlog.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Hello World!");

    // auto falcon = Falcon::Connect("127.0.0.1", 5556);
    // std::string message = "Hello World!";
    // std::span data(message.data(), message.size());
    // falcon->SendTo("127.0.0.1", 5555, data);
    //
    // std::string from_ip;
    // from_ip.resize(255);
    // std::array<char, 65535> buffer;
    // falcon->ReceiveFrom(from_ip, buffer);

    FalconClient falcon_client;

    falcon_client.OnConnectionEvent([](bool success, uint64_t client) {
        if (success) {
            std::cout << "Connected to server with client id: " << client << std::endl;
        } else {
            std::cout << "Failed to connect to server" << std::endl;
        }
    });

    falcon_client.ConnectTo("127.0.0.1", 5556);

    return EXIT_SUCCESS;
}
