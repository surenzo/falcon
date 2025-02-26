﻿
#include <iostream>
#include "../inc/falcon_API.h"
#include <cstring>
#include <array>
#include <utility>
#include <csignal>


FalconClient::FalconClient(): m_streams() {

    Falcon();
}

FalconClient::~FalconClient() {

    Falcon::~Falcon();
}

std::unique_ptr<FalconClient> FalconClient::ConnectTo(const std::string& ip, uint16_t port) {
    std::string new_ip;
    uint16_t new_port = 0;
    std::tie(new_ip, new_port) = GetClientAddress(ip);

    auto m_falcon = std::make_unique<FalconClient>();
    m_falcon->Connect("127.0.0.1", port);
    if (!m_falcon) {
        std::cerr << "Erreur : Impossible de se connecter à " << ip << ":" << port << std::endl;
        return nullptr;
    }
    m_falcon->StartListening();
    m_falcon->m_ip = new_ip;
    m_falcon->m_port =new_port;

    m_falcon->m_lastPing = std::chrono::steady_clock::now();
    m_falcon->m_lastPong = std::chrono::steady_clock::now();

    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Connect);
    std::vector<char> connectMessage = { static_cast<char>(protocolType) };

    m_falcon->SendTo(new_ip, new_port, std::span(connectMessage.data(), connectMessage.size()));
    return m_falcon;
}


void FalconClient::Update() {

    if (std::chrono::steady_clock::now() - m_lastPong > std::chrono::seconds(2) &&
        std::chrono::steady_clock::now() - m_lastPing > std::chrono::seconds(2)) {
        uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Ping);
        std::vector<char> pingMessage = { static_cast<char>(protocolType) };
        SendTo(m_ip, m_port, std::span(pingMessage.data(), pingMessage.size()));
        m_lastPing = std::chrono::steady_clock::now();
    }
    if (std::chrono::steady_clock::now() - m_lastPong > std::chrono::seconds(10)) {
        if (m_disconnectHandler) {
            m_disconnectHandler();
        }
        raise(SIGABRT);
    }

    for (auto& [streamId, streamMap] : m_streams) {
        for (auto& [isServer, stream] : streamMap) {
            stream->Update();
        }
    }

    while (true) {
        auto message = GetNextMessage();
        if (!message.has_value()) break;

        auto buffer = message->data;
        auto from_ip = message->from_ip;
        auto recv_size = message->recv_size;


        uint8_t protocolType = buffer.data()[0];

        switch (static_cast<ProtocolType>(protocolType)) {
            case ProtocolType::Connect:
                HandleConnectMessage(buffer);
            break;

            case ProtocolType::Stream:
                HandleStreamMessage(buffer);
            break;

            case ProtocolType::Ping:
                HandlePing(buffer);
            break;

            case ProtocolType::Pong:
                m_lastPong = std::chrono::steady_clock::now();
            break;

            case ProtocolType::StreamConnect:
                HandleStreamConnect(buffer);
                break;

            case ProtocolType::Acknowledgement:
                HandleAcknowledgement(from_ip, buffer);
            break;
            default:
                std::cerr << "Paquet inconnu reçu" << std::endl;
            break;
        }

    }
}

void FalconClient::HandleAcknowledgement(const std::string& from_ip, const std::vector<char>& buffer) {
    uint64_t clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;
    uint32_t packetId = *reinterpret_cast<const uint32_t*>(&buffer[14]);

    // Access the appropriate Stream object and call its Acknowledge method
    if (m_streams[streamId][serverStream]) {
        m_streams[streamId][serverStream]->Acknowledge(packetId);
    } else {
        std::cerr << "Error: Stream not found for clientId: " << clientId << ", streamId: " << streamId << ", serverStream: " << serverStream << std::endl;
    }

}
void FalconClient::HandleConnectMessage(const std::vector<char>& buffer) {
    m_clientId = *reinterpret_cast<const UUID*>(&buffer[1]);

    if (m_connectionHandler) {
        m_connectionHandler(true, m_clientId);
    }
}




void FalconClient::HandleStreamMessage(const std::vector<char>& buffer) {

    uint64_t clientId = *reinterpret_cast<const uint64_t*>(&buffer[1]);
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;

    m_streams[streamId][serverStream]->OnDataReceived(buffer);
    
}

void FalconClient::HandleStreamConnect(const std::vector<char>& buffer) {
    uint8_t flags = *reinterpret_cast<const uint8_t*>(&buffer[1]);
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[2]);


    auto stream = std::make_shared<Stream>(*this, m_clientId, streamId, flags & RELIABLE_MASK , m_ip, m_port,true);

    m_streams[streamId][true]= stream;
    m_newStreamHandler(stream);

}

void FalconClient::HandlePing(const std::vector<char>& buffer) {

    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Pong);
    std::vector<char> pongMessage = { static_cast<char>(protocolType) };
    SendTo(m_ip, m_port, std::span(pongMessage.data(), pongMessage.size()));
}



void FalconClient::OnConnectionEvent(std::function<void(bool, UUID)> handler){
    m_connectionHandler = std::move(handler);
}

void FalconClient::OnDisconnect(std::function<void()> handler){
    m_disconnectHandler = std::move(handler);
}

void FalconClient::OnCreateStream(std::function<void(std::shared_ptr<Stream>)> handler) {
    m_newStreamHandler = std::move(handler);
}


std::shared_ptr<Stream> FalconClient::CreateStream(bool reliable) {
    Falcon* falcon = this;
    uint32_t streamId = m_nextStreamId++;
    auto stream = std::make_shared<Stream>(*falcon, m_clientId, streamId, reliable, m_ip, m_port);
    m_streams[streamId][false] = stream;
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::StreamConnect);
    uint8_t flags = reliable ? RELIABLE_MASK : 0;
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&m_clientId), reinterpret_cast<const char*>(&m_clientId) + sizeof(m_clientId));
    message.insert(message.end(), flags);
    message.insert(message.end(), reinterpret_cast<const char*>(&streamId), reinterpret_cast<const char*>(&streamId) + sizeof(streamId));
    SendTo(m_ip, m_port, std::span(message.data(), message.size()));
    return stream;
}