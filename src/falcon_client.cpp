
#include <iostream>
#include "../inc/falcon_API.h"
#include <cstring>


FalconClient::FalconClient() {
    // call the constructor of the base class
    Falcon();
    // 🔹 Initialiser les streams
    m_streams = std::unordered_map<uint64_t, std::unique_ptr<Stream>>();
}

FalconClient::~FalconClient() {
    // call the destructor of the base class
    Falcon::~Falcon();
}

void FalconClient::ConnectTo(const std::string& ip, uint16_t port) {

    std::string new_ip = ip;
    uint16_t new_port = 0;
    auto pos = ip.find_last_of (':');
    if (pos != std::string::npos) {
        new_ip = ip.substr (0,pos);
        std::string port_str = ip.substr (++pos);
        new_port = atoi(port_str.c_str());
    }

    auto falcon = Falcon::Connect(new_ip, port);
    if (!falcon) {
        std::cerr << "Erreur : Impossible de se connecter à " << ip << ":" << port << std::endl;
        return;
    }

    // 🔥 Étape 1 : Envoyer un paquet de connexion (0x00)
    uint8_t connectionPacket = 0x00;
    falcon->SendTo(new_ip, new_port, std::span {reinterpret_cast<const char*>(&connectionPacket), sizeof(connectionPacket)});

    // 🔥 Étape 2 : Attendre un UUID du serveur
    std::string from_ip;
    from_ip.resize(255);
    std::array<char, 65535> buffer;
    int recv_size = falcon->ReceiveFrom(from_ip, buffer);
    printf("Received %d bytes\n", recv_size);

    if (recv_size == sizeof(UUID)) {
        memcpy(&m_clientId, buffer.data(), sizeof(UUID));

        if (m_connectionHandler) {
            m_connectionHandler(true, m_clientId);
        }
    } else {
        if (m_connectionHandler) {
            m_connectionHandler(false, 0);
        }
    }
}

void FalconClient::OnConnectionEvent(std::function<void(bool, UUID)> handler){
    m_connectionHandler = handler;
}

void FalconClient::OnDisconnect(std::function<void()> handler){
    m_disconnectHandler = handler;
}

std::unique_ptr<Stream> FalconClient::CreateStream(bool reliable) {
    // 🔥 Créer un flux
    return nullptr;
}
