#include <iostream>

#include <falcon.h>
#include <falcon_API.h>

#include "spdlog/spdlog.h"

int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Hello World!");

    auto falconServer = FalconServer::ListenTo(5555);
    falconServer->OnClientConnected([&](UUID client) {
        std::cout << "Client connected: " << client << std::endl;
        auto stream = falconServer->CreateStream(client, true);
        stream->OnDataReceivedHandler([](std::span<const char> data) {
            std::string message(data.begin(), data.end());
            std::cout << "Received message(server stream): " << message << std::endl;
        });
        std::cout << "Send Quoi ? to client through server stream" << std::endl;
        std::string message = "Quoi ?";
        stream->SendData(std::span(message.data(), message.size()));;
    });

    falconServer->OnClientDisconnected([](UUID client) {
        std::cout << "Client disconnected: " << client << std::endl;
    });

    falconServer->OnCreateStream([](std::shared_ptr<Stream> stream) {
        stream->OnDataReceivedHandler([](std::span<const char> data) {
            std::string message(data.begin(), data.end());
            std::cout << "Received message(server stream): " << message << std::endl;
        });
        std::cout << "Send General Kenobi! to client through client stream" << std::endl;
        std::string message = "General Kenobi!";
        stream->SendData(std::span(message.data(), message.size()));
    });


    while (true) {
        falconServer->Update();
    }

    return EXIT_SUCCESS;
}
