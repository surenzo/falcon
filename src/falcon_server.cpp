#include <iostream>
#include "../inc/falcon_API.h"
#include <random>
#include <array>

FalconServer::FalconServer() {
    Falcon();
    clients = std::unordered_map<std::string, UUID>();
    m_clients = std::unordered_map<UUID, std::chrono::steady_clock::time_point>();
}

FalconServer::~FalconServer() {
  
    Falcon::~Falcon();
}

UUID GenerateUUID() {
    static std::random_device rd;  
    static std::mt19937_64 eng(rd()); 
    static std::uniform_int_distribution<uint64_t> dist; 
    return dist(eng); 
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

        int recv_size = falcon->ReceiveFrom(from_ip, std::span<char, 65535>(buffer));
        if (recv_size <= 0) continue;

    
        std::string ip = from_ip;
        uint16_t new_port = 0;
        auto pos = from_ip.find_last_of(':');
        if (pos != std::string::npos) {
            ip = from_ip.substr(0, pos);
            std::string port_str = from_ip.substr(++pos);
            new_port = atoi(port_str.c_str());
        }

        printf("Received %d bytes from %s:%d\n", recv_size, ip.c_str(), new_port);

        uint8_t protocolType = buffer.data()[0];

        switch (static_cast<ProtocolType>(protocolType)) {
            case ProtocolType::Connect:
                
                HandleConnect(falcon, ip, buffer, new_port);
                break;

            case ProtocolType::Acknowledgement:
                
                HandleAcknowledgement(ip, buffer);
                break;

            case ProtocolType::Reco:
               
                HandleReco(ip, buffer);
                break;

            case ProtocolType::Stream:
               
                HandleStream(falcon, ip, buffer);
                break;

            default:
                std::cerr << "Paquet inconnu reçu" << std::endl;
                break;
        }
    }
}

void FalconServer::HandleConnect(std::unique_ptr<Falcon>& falcon, const std::string& from_ip, const std::array<char, 65535>& buffer, uint16_t port) {
    if (clients.find(from_ip) == clients.end()) {
        UUID clientId = GenerateUUID(); 
        clients[from_ip] = clientId;
        m_clients[clientId] = std::chrono::steady_clock::now();  
        m_clientIdToAddress[clientId] = std::make_pair(from_ip, port);

        // Notify that a new client has connected.
        if (m_clientConnectedHandler) {
            m_clientConnectedHandler(clientId);
        }
        uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Connect);
        std::vector<char> message = { static_cast<char>(protocolType) };
        message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));

        falcon->SendTo(from_ip, port, std::span(message.data(), message.size()));
    }
}

void FalconServer::SendAcknowledgement(std::unique_ptr<Falcon>& falcon, const std::string& from_ip, uint64_t clientId) {
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Acknowledgement);
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));

    falcon->SendTo(from_ip, 5555, std::span(message.data(), message.size()));
}

void FalconServer::SendRecoMessage(std::unique_ptr<Falcon>& falcon, const std::string& from_ip, uint64_t clientId) {
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Reco);
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));

    falcon->SendTo(from_ip, 5555, std::span(message.data(), message.size()));
}

void FalconServer::SendStreamData(std::unique_ptr<Falcon>& falcon,const std::string& from_ip, uint32_t streamId, const std::vector<char>& data) {
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Stream);
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&streamId), reinterpret_cast<const char*>(&streamId) + sizeof(streamId));

    message.insert(message.end(), data.begin(), data.end());
    falcon->SendTo(from_ip, 5555, std::span(message.data(), message.size()));
}

void FalconServer::HandleAcknowledgement(const std::string& from_ip, const std::array<char, 65535>& buffer) {
    
    std::cout << "Acknowledgment packet received from " << from_ip << std::endl;
}

void FalconServer::HandleReco(const std::string& from_ip, const std::array<char, 65535>& buffer) {
    
    std::cout << "Reconnection request received from " << from_ip << std::endl;
}

void FalconServer::HandleStream(std::unique_ptr<Falcon>& falcon, const std::string& from_ip, const std::array<char, 65535>& buffer) {
    
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[1]);  // Extract stream ID
    std::span<const char> data(buffer.data() + sizeof(streamId), buffer.size() - sizeof(streamId));

    std::cout << "Stream ID " << streamId << " received from " << from_ip << std::endl;

    
}



std::unique_ptr<Stream> FalconServer::CreateStream(UUID clientId, bool reliable) {
    Falcon* falcon = static_cast<Falcon*>(this);
    uint32_t streamId = m_nextStreamId++;
    auto stream = std::make_unique<Stream>(*falcon, clientId, streamId, reliable, m_clientIdToAddress.at(clientId).first, m_clientIdToAddress.at(clientId).second);
    m_streams[clientId][streamId] = std::move(stream);
    return std::move(m_streams[clientId][streamId]);
}

void FalconServer::OnClientConnected(std::function<void(UUID)> handler) {
    m_clientConnectedHandler = handler;
}

void FalconServer::OnClientDisconnected(std::function<void(UUID)> handler) {
    m_clientDisconnectedHandler = handler;
}

void FalconServer::CloseStream(const Stream& stream) {
    m_streams[stream.GetClientId()].erase(stream.GetStreamId());
    stream.~Stream();
}

std::pair<std::string, uint16_t> FalconServer::GetClientAddress(uint64_t clientId) const {
    return m_clientIdToAddress.at(clientId);
}