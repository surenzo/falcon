#include <iostream>
#include "../inc/falcon_API.h"
#include <random>
#include <array>

FalconServer::FalconServer() : m_streams(){
    Falcon();
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

std::unique_ptr<FalconServer> FalconServer::ListenTo(uint16_t port) {
    auto falcon = std::make_unique<FalconServer>();
    falcon->Listen("127.0.0.1", port);
    if (!falcon) {
        std::cerr << "Erreur : Impossible d'écouter sur le port " << port << std::endl;
        return nullptr;
    }

    falcon->StartListening();

    return falcon;
}

void FalconServer::Update() {
    // ping the server if the time exceeds 0.1s without receiving any message
    // for any client in the uuid map that has not been pinged for 1 second, disconnect the client

    //  create pool to erase in the map
    std::vector<UUID> to_erase;
    for (auto& [clientId, lastPing] : m_clients_Ping) {
        if (std::chrono::steady_clock::now() - m_clients_Pong[clientId] > std::chrono::seconds(2)&&
            std::chrono::steady_clock::now() - lastPing > std::chrono::seconds(2) ) {

            uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Ping);
            std::vector<char> pingMessage = { static_cast<char>(protocolType) };

            m_clients_Ping[clientId] = std::chrono::steady_clock::now();

            SendTo(m_clientIdToAddress[clientId].first,
                m_clientIdToAddress[clientId].second,
                std::span(pingMessage.data(),
                pingMessage.size()));
        }
        if (std::chrono::steady_clock::now() - m_clients_Pong[clientId] > std::chrono::seconds(10)) {
            if (m_clientDisconnectedHandler) {
                m_clientDisconnectedHandler(clientId);
            }
            to_erase.push_back(clientId);
        }
    }
    for (auto& clientId : to_erase) {
        m_clients_Ping.erase(clientId);
        m_clients_Pong.erase(clientId);
        m_clientIdToAddress.erase(clientId);
        for (auto it = clients.begin(); it != clients.end(); ) {
            if (it->second == clientId) {
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }
    while (true) {
        auto message = GetNextMessage();
        if (!message.has_value()) break;

        auto& [data, ip, recv_size] = message.value();

        std::vector<char> buffer(recv_size);
        std::ranges::copy(data.subspan(0,recv_size), buffer.begin());

        uint8_t protocolType = buffer.data()[0];

        switch (static_cast<ProtocolType>(protocolType)) {
            case ProtocolType::Connect:

                HandleConnect(ip, buffer);
            break;

            /*case ProtocolType::Acknowledgement:

                HandleAcknowledgement(ip, buffer);
            break;*/

            case ProtocolType::Stream:
                HandleStream(ip, buffer);
            break;

            case ProtocolType::StreamConnect:
                HandleStreamConnect(buffer);

            case ProtocolType::Ping:
                HandlePing(ip, buffer);
            break;

            case ProtocolType::Pong:
                m_clients_Pong[clients[ip]] = std::chrono::steady_clock::now();
            break;

            default:
                std::cerr << "Paquet inconnu reçu" << std::endl;
            break;
        }
    }
}


void FalconServer::HandleConnect(const std::string& ip, const std::vector<char>& buffer){
    std::string new_ip;
    uint16_t new_port = 0;
    std::tie(new_ip, new_port) = GetClientAddress(ip);
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Connect);
    std::vector<char> message = { static_cast<char>(protocolType) };
    if (clients.find(ip) == clients.end()) {

        UUID clientId = GenerateUUID(); 
        clients[ip] = clientId;
        m_clients_Pong[clientId] = std::chrono::steady_clock::now();
        m_clientIdToAddress[clientId] = std::make_pair(new_ip, new_port);

        message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));

        m_clients_Ping[clientId] = std::chrono::steady_clock::now();
        SendTo(new_ip, new_port, std::span(message.data(), message.size()));
        // Notify that a new client has connected.
        if (m_clientConnectedHandler) {
            m_clientConnectedHandler(clientId);
        }
    }
    else {
        uint64_t clientId = clients[ip];
        message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));

        SendTo(new_ip, new_port, std::span(message.data(), message.size()));
    }
}

/*void FalconServer::SendAcknowledgement( const std::string& from_ip, uint64_t clientId) {
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Acknowledgement);
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));

    SendTo(from_ip, 5555, std::span(message.data(), message.size()));
}*/


void FalconServer::SendStreamData(const std::string& from_ip, uint32_t streamId, const std::vector<char>& data) {
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Stream);
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&streamId), reinterpret_cast<const char*>(&streamId) + sizeof(streamId));

    message.insert(message.end(), data.begin(), data.end());
    SendTo(from_ip, 5555, std::span(message.data(), message.size()));
}

/*void FalconServer::HandleAcknowledgement(const std::string& from_ip, const std::array<char, 65535>& buffer) {
    
    std::cout << "Acknowledgment packet received from " << from_ip << std::endl;
}*/

void FalconServer::HandleStream( const std::string& from_ip, const std::vector<char>& buffer) {

    uint64_t clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);  // Extract client ID
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);  // Extract stream ID
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;  // Extract server stream

    //if (!m_streams[std::make_pair(clientId,serverStream)][streamId])
        // creer ?

    std::cout << "Stream received from " << from_ip << " with clientId: " << clientId << ", streamId: " << streamId << ", serverStream: " << serverStream << std::endl;

    if (m_streams[clientId][streamId][serverStream]) {
        m_streams[clientId][streamId][serverStream]->OnDataReceived(buffer);
    } else {
        std::cerr << "Error: Stream not initialized for clientId: " << clientId << ", streamId: " << streamId << ", serverStream: " << serverStream << std::endl;
        // Handle the error appropriately, e.g., by initializing the stream or returning an error code
    }
}

void FalconServer::HandleStreamConnect(const std::vector<char>& buffer) {
    uint64_t clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    uint8_t flags = *reinterpret_cast<const uint8_t*>(&buffer[9]);
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[10]);

    std::string new_ip = m_clientIdToAddress[clientId].first;
    uint16_t new_port = m_clientIdToAddress[clientId].second;

    auto stream = std::make_shared<Stream>(*this, clientId, streamId, flags & RELIABLE_MASK , new_ip, new_port,false);
    std::cout << "Stream created for client " << clientId << " with streamId " << streamId << std::endl;
    m_streams[clientId][streamId][false] = stream;
    m_newStreamHandler(stream);
}

void FalconServer::HandlePing(const std::string &from_ip, const std::vector<char>& buffer) {
    uint64_t clientId = *reinterpret_cast<const uint32_t*>(&buffer[1]);
    m_clients_Pong[clientId] = std::chrono::steady_clock::now();


    ProtocolType protocolType = ProtocolType::Pong;
    std::vector<char> message = { static_cast<char>(protocolType) };
    std::string new_ip;
    uint16_t new_port = 0;
    std::tie(new_ip, new_port) = GetClientAddress(from_ip);
    SendTo(new_ip, new_port, std::span(message.data(), message.size()));
}




std::shared_ptr<Stream> FalconServer::CreateStream(UUID clientId, bool reliable) {
    Falcon* falcon = this;
    uint32_t streamId = m_nextStreamId++;
    std::string ip = m_clientIdToAddress[clientId].first;
    uint16_t port = m_clientIdToAddress[clientId].second;
    auto stream = std::make_shared<Stream>(*falcon, clientId, streamId, reliable, ip, port);
    m_streams[clientId][streamId][true] = stream;
    // send a notification to the client to create the stream on its side
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::StreamConnect);
    uint8_t flags = reliable ? RELIABLE_MASK : 0;
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.push_back(flags);
    message.insert(message.end(), reinterpret_cast<const char*>(&streamId), reinterpret_cast<const char*>(&streamId) + sizeof(streamId));
    SendTo(ip, port, std::span(message.data(), message.size()));
    return stream;
}

void FalconServer::OnClientConnected(std::function<void(UUID)> handler) {
    m_clientConnectedHandler = handler;
}

void FalconServer::OnClientDisconnected(std::function<void(UUID)> handler) {
    m_clientDisconnectedHandler = handler;
}

void FalconServer::OnCreateStream(std::function<void(std::shared_ptr<Stream>)> handler) {
    m_newStreamHandler = std::move(handler);
}

void FalconServer::CloseStream(const Stream& stream) {
    stream.~Stream();
}
