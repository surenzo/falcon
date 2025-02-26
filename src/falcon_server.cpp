#include <iostream>
#include <random>
#include <array>
#include <utility>
#include "../inc/falcon_API.h"

FalconServer::FalconServer() : Falcon(), m_streams() {
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
    falcon->StartListening();
    return falcon;
}

void FalconServer::Update() {
    std::vector<UUID> to_erase;
    auto now = std::chrono::steady_clock::now();

    for (auto& [clientId, lastPing] : m_clients_Ping) {
        if (now - m_clients_Pong[clientId] > std::chrono::seconds(2) && now - lastPing > std::chrono::seconds(2)) {
            SendPing(clientId);
        }
        if (now - m_clients_Pong[clientId] > std::chrono::seconds(10)) {
            HandleClientDisconnection(clientId, to_erase);
        }
    }

    RemoveDisconnectedClients(to_erase);
    UpdateStreams();
    ProcessMessages();
}

void FalconServer::SendPing(UUID clientId) {
    auto protocolType = static_cast<uint8_t>(ProtocolType::Ping);
    std::vector pingMessage = { static_cast<char>(protocolType) };
    m_clients_Ping[clientId] = std::chrono::steady_clock::now();
    SendTo(m_clientIdToAddress[clientId].first, m_clientIdToAddress[clientId].second, std::span(pingMessage.data(), pingMessage.size()));
}

void FalconServer::HandleClientDisconnection(UUID clientId, std::vector<UUID>& to_erase) const {
    if (m_clientDisconnectedHandler) {
        m_clientDisconnectedHandler(clientId);
    }
    to_erase.push_back(clientId);
}

void FalconServer::RemoveDisconnectedClients(const std::vector<UUID>& to_erase) {
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
}

void FalconServer::UpdateStreams() {
    for (auto& [clientId, streamMap] : m_streams) {
        for (auto& [streamId, streamSubMap] : streamMap) {
            for (auto& [isServer, stream] : streamSubMap) {
                stream->Update();
            }
        }
    }
}

void FalconServer::ProcessMessages() {
    while (true) {
        auto message = GetNextMessage();
        if (!message.has_value()) break;

        auto buffer = message->data;
        auto ip = message->from_ip;

        uint8_t protocolType = buffer[0];
        switch (static_cast<ProtocolType>(protocolType)) {
            case ProtocolType::Connect:
                HandleConnect(ip);
                break;
            case ProtocolType::Ping:
                HandlePing(buffer);
                break;
            case ProtocolType::Pong:
                m_clients_Pong[clients[ip]] = std::chrono::steady_clock::now();
                break;
            case ProtocolType::Stream:
                HandleStream(buffer);
                break;
            case ProtocolType::StreamConnect:
                HandleStreamConnect(buffer);
                break;
            case ProtocolType::Acknowledgement:
                HandleAcknowledgement(buffer);
                break;
        }
    }
}


void FalconServer::HandleConnect(const std::string& ip) {
    auto [new_ip, new_port] = GetClientAddress(ip);
    auto protocolType = static_cast<uint8_t>(ProtocolType::Connect);

    auto clientId = GenerateUUID();

    clients[ip] = clientId;
    m_clients_Ping[clientId] = std::chrono::steady_clock::now();
    m_clients_Pong[clientId] = std::chrono::steady_clock::now();
    m_clientIdToAddress[clientId] = std::make_pair(new_ip, new_port);

    std::vector message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&clientId), reinterpret_cast<const char*>(&clientId) + sizeof(clientId));
    SendTo(new_ip, new_port, std::span(message.data(), message.size()));

    if (m_clientConnectedHandler) {
        m_clientConnectedHandler(clientId);
    }
}

void FalconServer::HandlePing(const std::vector<char>& buffer) {
    auto clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    m_clients_Pong[clientId] = std::chrono::steady_clock::now();

    ProtocolType protocolType = ProtocolType::Pong;
    std::vector message = { static_cast<char>(protocolType) };

    auto [new_ip, new_port] = m_clientIdToAddress[clientId];
    SendTo(new_ip, new_port, std::span(message.data(), message.size()));
}

void FalconServer::HandleStream(const std::vector<char>& buffer) {
    auto clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    auto streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;

    m_streams[clientId][streamId][serverStream]->OnDataReceived(buffer);
}

void FalconServer::HandleStreamConnect(const std::vector<char>& buffer) {
    auto clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    auto flags = *reinterpret_cast<const uint8_t*>(&buffer[9]);
    auto streamId = *reinterpret_cast<const uint32_t*>(&buffer[10]);
    auto [new_ip, new_port] = m_clientIdToAddress[clientId];

    auto stream = std::make_shared<Stream>(*this, clientId, streamId, flags & RELIABLE_MASK, new_ip, new_port);
    m_streams[clientId][streamId][false] = stream;
    m_newStreamHandler(stream);
}

void FalconServer::HandleAcknowledgement(const std::vector<char>& buffer) {
    auto clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    auto streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;
    auto packetId = *reinterpret_cast<const uint32_t*>(&buffer[14]);

    m_streams[clientId][streamId][serverStream]->Acknowledge(packetId);
}


std::shared_ptr<Stream> FalconServer::CreateStream(UUID clientId, bool reliable) {
    auto streamId = m_nextStreamId++;
    auto [ip, port] = m_clientIdToAddress[clientId];
    auto stream = std::make_shared<Stream>(*this, clientId, streamId, reliable, ip, port, true);
    m_streams[clientId][streamId][true] = stream;

    constexpr auto protocolType = static_cast<uint8_t>(ProtocolType::StreamConnect);
    const uint8_t flags = reliable ? RELIABLE_MASK : 0;
    std::vector message = { static_cast<char>(protocolType) };
    message.insert(message.end(), flags);
    message.insert(message.end(), reinterpret_cast<const char*>(&streamId), reinterpret_cast<const char*>(&streamId) + sizeof(streamId));
    SendTo(ip, port, std::span(message.data(), message.size()));

    return stream;
}

void FalconServer::OnClientConnected(std::function<void(UUID)> handler) {
    m_clientConnectedHandler = std::move(handler);
}

void FalconServer::OnClientDisconnected(std::function<void(UUID)> handler) {
    m_clientDisconnectedHandler = std::move(handler);
}

void FalconServer::OnCreateStream(std::function<void(std::shared_ptr<Stream>)> handler) {
    m_newStreamHandler = std::move(handler);
}

void FalconServer::CloseStream(const Stream& stream) {
    auto clientId = stream.GetClientId();
    auto streamId = stream.GetStreamId();
    m_streams[clientId].erase(streamId);
    stream.~Stream();
}
