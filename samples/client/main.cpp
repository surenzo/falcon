#include <iostream>

#include <falcon_API.h>

#include "spdlog/spdlog.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Hello World!");


    auto falcon_client = FalconClient::ConnectTo("127.0.0.1:5555", 5556);

    falcon_client->OnConnectionEvent([&](bool success, UUID client) {
    if (success) {
        std::cout << "Connected to server with client id: " << client << std::endl;
        auto stream = falcon_client->CreateStream(true);
        stream->OnDataReceivedHandler([](std::span<const char> data) {
            std::string message(data.begin(), data.end());
            std::cout << "Received message ( stream): " << message << std::endl;
        });
        std::cout << "Send Hello There! to server through client stream" << std::endl;
        std::string message = "Hello There!";
        stream->SendData(std::span(message.data(), message.size()));;
    } else {
        std::cout << "Failed to connect to server" << std::endl;
    }
    });

    falcon_client->OnCreateStream([](std::shared_ptr<Stream> stream) {
        stream->OnDataReceivedHandler([](std::span<const char> data) {
            std::string message(data.begin(), data.end());
            std::cout << "Received message(server stream): " << message << std::endl;
        });
        std::cout << "Send Feur to server through server stream" << std::endl;
        std::string message = "Feur";
        stream->SendData(std::span(message.data(), message.size()));
    });


    while (true) {
        falcon_client->Update();
    }

    return EXIT_SUCCESS;
}
