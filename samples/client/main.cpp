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

    auto falcon_client = FalconClient::ConnectTo("127.0.0.1:5555", 5556);

    falcon_client->OnConnectionEvent([](bool success, UUID client) {
    if (success) {
        std::cout << "Connected to server with client id: " << client << std::endl;
    } else {
        std::cout << "Failed to connect to server" << std::endl;
    }
    });

    // when a stream is created by the server, you send a message through the stream
    falcon_client->OnCreateStream([](std::shared_ptr<Stream> stream) {
        std::string message = "Hello World!";
        //wait for 1s
        std::this_thread::sleep_for(std::chrono::seconds(1));
        stream->SendData(std::span(message.data(), message.size()));
    });


    while (true) {
        falcon_client->Update();
    }

    return EXIT_SUCCESS;
}
