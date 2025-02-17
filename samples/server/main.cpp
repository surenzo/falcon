#include <iostream>
#include <map>
#include <falcon.h>
#include <thread>
#include "spdlog/spdlog.h"

std::map <std::string, uint64_t> mapAddresses;

int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Hello World!");

    auto falcon = Falcon::Listen("127.0.0.1", 5555);
    std::string from_ip;
    from_ip.resize(255);
    std::array<char, 65535> buffer;
    int recv_size = falcon->ReceiveFrom(from_ip, buffer);
    std::string ip = from_ip;
    uint16_t port = 0;
    auto pos = from_ip.find_last_of (':');
    if (pos != std::string::npos) {
        ip = from_ip.substr (0,pos);
        std::string port_str = from_ip.substr (++pos);
        port = atoi(port_str.c_str());
    }
    falcon->SendTo(ip, port, std::span {buffer.data(), static_cast<unsigned long>(recv_size)});
    return EXIT_SUCCESS;
}

void Listen(uint16_t port) {
    //possiblement faire un fork en while true ?

    auto falcon = Falcon::Listen("127.0.0.1", port);
    while (true) {
        std::string from_ip;
        from_ip.resize(255);
        std::array<char, 65535> buffer;

        int recv_size = falcon->ReceiveFrom(from_ip, buffer);

        std::thread([falcon, from_ip, buffer, recv_size, port]() {

            std::string ip = from_ip;

            auto pos = from_ip.find_last_of (':');
            if (pos != std::string::npos) {
                ip = from_ip.substr (0,pos);
            }
            
            if (mapAddresses.find(ip) == mapAddresses.end()) {
                mapAddresses[ip] = port;
                //jouer onclientconnected en envoyant au client un message (
            }

        }).detach();
    }
}

void OnClientConnected(std::function<void(uint64_t)> handler) {

}

void OnClientDisconnected(std::function<void, uint64_t> handler); //Server API
