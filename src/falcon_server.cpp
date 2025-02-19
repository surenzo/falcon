
#include <iostream>
#include "../inc/falcon_API.h"


FalconServer::FalconServer() {
    // call the constructor of the base class
    Falcon();
    // 🔹 Initialiser les clients
    clients = std::unordered_map<std::string, uuid128_t>();
    m_clients = std::unordered_map<uuid128_t, std::chrono::steady_clock::time_point>();
}

FalconServer::~FalconServer() {
    // call the destructor of the base class
    Falcon::~Falcon();
}


uuid128_t GenerateUUID() {
    // 🔥 Générer un UUID
    return 0;
}

void FalconServer::Listen(uint16_t port) {
    auto falcon = Falcon::Listen("127.0.0.1", port);
    if (!falcon) {
        std::cerr << "Erreur : Impossible d'écouter sur le port " << port << std::endl;
        return;
    }

    std::cout << "Serveur en écoute sur le port " << port << std::endl;

    while (true) {
        std::string from_ip;
        from_ip.resize(255);
        std::array<char, 65535> buffer;

        int recv_size = falcon->ReceiveFrom(from_ip, buffer);
        if (recv_size <= 0) continue;

        // 🔹 Vérifier si c'est un paquet de connexion (0x00)
        if (buffer[0] == 0x00) {
            if (clients.find(from_ip) == clients.end()) {
                uint64_t clientId = GenerateUUID();
                clients[from_ip] = clientId;
                m_clients[clientId] = std::chrono::steady_clock::now();

                // 🔥 Notifier qu'un nouveau client est connecté
                if (m_clientConnectedHandler) {
                    m_clientConnectedHandler(clientId);
                }

                // 🔥 Envoyer l'UUID au client
                falcon->SendTo(from_ip, port, std::span(reinterpret_cast<const char*>(&clientId), sizeof(clientId)));
            }
        }
    }
}

std::unique_ptr<Stream> FalconServer::CreateStream(uuid128_t client, bool reliable) {
    // 🔥 Créer un flux
    return nullptr;
}

void FalconServer::OnClientConnected(std::function<void(uuid128_t)> handler) {
    m_clientConnectedHandler = handler;
}

void FalconServer::OnClientDisconnected(std::function<void(uuid128_t)> handler) {
    m_clientDisconnectedHandler = handler;
}

void FalconServer::CloseStream(const Stream& stream) {
    // 🔥 Fermer un flux
}