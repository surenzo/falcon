
#include <iostream>
#include "../inc/falcon_API.h"
#include <random>


FalconServer::FalconServer() {
    Falcon();

    clients = std::unordered_map<std::string, UUID>();
    m_clients = std::unordered_map<UUID, std::chrono::steady_clock::time_point>();
}

FalconServer::~FalconServer() {
    // call the destructor of the base class
    Falcon::~Falcon();
}


UUID GenerateUUID() {
    static std::random_device rd;  // Source de hasard matérielle (si dispo)
    static std::mt19937_64 eng(rd()); // Générateur de nombres pseudo-aléatoires 64 bits
    static std::uniform_int_distribution<uint64_t> dist; // Distribution uniforme sur 64 bits

    return dist(eng); // Retourne un nombre 64 bits aléatoire comme UUID
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


        std::string ip = from_ip;
        uint16_t new_port = 0;
        auto pos = from_ip.find_last_of (':');
        if (pos != std::string::npos) {
            ip = from_ip.substr (0,pos);
            std::string port_str = from_ip.substr (++pos);
            new_port = atoi(port_str.c_str());
        }

        printf("Received %d bytes from %s:%d\n", recv_size, ip.c_str(), new_port);


        // 🔹 Vérifier si c'est un paquet de connexion (0x00)
        if (buffer.data()[0] == 0x00) {
            if (clients.find(ip) == clients.end()) {
                UUID clientId = GenerateUUID();
                clients[ip] = clientId;
                m_clients[clientId] = std::chrono::steady_clock::now();

                // 🔥 Notifier qu'un nouveau client est connecté
                if (m_clientConnectedHandler) {
                    m_clientConnectedHandler(clientId);
                }

                // 🔥 Envoyer l'UUID au client
                falcon->SendTo(ip, new_port, std::span(reinterpret_cast<const char*>(&clientId), sizeof(clientId)));
            }
        }
    }
}

std::unique_ptr<Stream> FalconServer::CreateStream(UUID client, bool reliable) {
    // 🔥 Créer un flux
    return nullptr;
}

void FalconServer::OnClientConnected(std::function<void(UUID)> handler) {
    m_clientConnectedHandler = handler;
}

void FalconServer::OnClientDisconnected(std::function<void(UUID)> handler) {
    m_clientDisconnectedHandler = handler;
}

void FalconServer::CloseStream(const Stream& stream) {
    // 🔥 Fermer un flux
}