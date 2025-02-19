
#include <iostream>
#include "../inc/falcon_API.h"


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
    auto falcon = Falcon::Connect(ip, port);
    if (!falcon) {
        std::cerr << "Erreur : Impossible de se connecter à " << ip << ":" << port << std::endl;
        return;
    }

    // 🔥 Étape 1 : Envoyer un paquet de connexion (0x00)
    uint8_t connectionPacket = 0x00;
    falcon->SendTo(ip, port, std::span {reinterpret_cast<const char*>(&connectionPacket), sizeof(connectionPacket)});

    // 🔥 Étape 2 : Attendre un UUID du serveur
    std::string from_ip = ip;
    from_ip.resize(255);
    std::array<char, 65535> buffer;
    int recv_size = falcon->ReceiveFrom(from_ip, buffer);

    if (recv_size == sizeof(uuid128_t)) {
        memcpy(&m_clientId, buffer.data(), sizeof(uuid128_t));

        if (m_connectionHandler) {
            m_connectionHandler(true, m_clientId);
        }
    } else {
        if (m_connectionHandler) {
            m_connectionHandler(false, 0);
        }
    }
}

void FalconClient::OnConnectionEvent(std::function<void(bool, uint64_t)> handler){
    m_connectionHandler = handler;
}

void FalconClient::OnDisconnect(std::function<void()> handler){
    m_disconnectHandler = handler;
}

std::unique_ptr<Stream> FalconClient::CreateStream(bool reliable) {
    // 🔥 Créer un flux
    return nullptr;
}
