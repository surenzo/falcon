#include <iostream>
#include <cstring>
#include <array>
#include <utility>
#include <csignal>
#include "../inc/falcon_API.h"

FalconClient::FalconClient() :Falcon(), m_clientId(), m_port(), m_streams() {}

FalconClient::~FalconClient() {
    Falcon::~Falcon();
}

std::unique_ptr<FalconClient> FalconClient::ConnectTo(const std::string& ip, uint16_t port) {
    auto [new_ip, new_port] = GetClientAddress(ip);
    auto m_falcon = std::make_unique<FalconClient>();
    m_falcon->Connect("127.0.0.1", port);
    m_falcon->StartListening();
    m_falcon->m_ip = new_ip;
    m_falcon->m_port = new_port;

    m_falcon->m_lastPing = std::chrono::steady_clock::now();
    m_falcon->m_lastPong = std::chrono::steady_clock::now();

    auto protocolType = static_cast<uint8_t>(ProtocolType::Connect);
    std::vector connectMessage = { static_cast<char>(protocolType) };

    m_falcon->SendTo(new_ip, new_port, std::span(connectMessage.data(), connectMessage.size()));
    return m_falcon;
}

void FalconClient::Update() {
    auto now = std::chrono::steady_clock::now();

    if (now - m_lastPong > std::chrono::seconds(2) && now - m_lastPing > std::chrono::seconds(2)) {
        SendPing();
    }
    if (now - m_lastPong > std::chrono::seconds(10)) {
        HandleDisconnection();
    }

    UpdateStreams();
    ProcessMessages();
}

void FalconClient::SendPing() {
    auto protocolType = static_cast<uint8_t>(ProtocolType::Ping);
    std::vector pingMessage = { static_cast<char>(protocolType) };
    SendTo(m_ip, m_port, std::span(pingMessage.data(), pingMessage.size()));
    m_lastPing = std::chrono::steady_clock::now();
}

void FalconClient::HandleDisconnection() {
    if (m_disconnectHandler) {
        m_disconnectHandler();
    }
    raise(SIGABRT);
}

void FalconClient::UpdateStreams() {
    for (auto& [streamId, streamMap] : m_streams) {
        for (auto& [isServer, stream] : streamMap) {
            stream->Update();
        }
    }
}

void FalconClient::ProcessMessages() {
    while (true) {
        auto message = GetNextMessage();
        if (!message.has_value()) break;

        auto buffer = message->data;
        auto from_ip = message->from_ip;

        m_lastPong = std::chrono::steady_clock::now();


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

void FalconClient::HandleAcknowledgement(const std::vector<char>& buffer) {
    auto streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;
    auto packetId = *reinterpret_cast<const uint32_t*>(&buffer[14]);

    m_streams[streamId][serverStream]->Acknowledge(packetId);
}

void FalconClient::HandleConnectMessage(const std::vector<char>& buffer) {
    m_clientId = *reinterpret_cast<const UUID*>(&buffer[1]);

    if (m_connectionHandler) {
        m_connectionHandler(true, m_clientId);
    }
}

void FalconClient::HandleStreamMessage(const std::vector<char>& buffer) {
    auto streamId = *reinterpret_cast<const uint32_t*>(&buffer[9]);
    bool serverStream = buffer[13] & CLIENT_STREAM_MASK;

    m_streams[streamId][serverStream]->OnDataReceived(buffer);

}

void FalconClient::HandleStreamConnect(const std::vector<char>& buffer) {
    auto flags = *reinterpret_cast<const uint8_t*>(&buffer[1]);
    auto streamId = *reinterpret_cast<const uint32_t*>(&buffer[2]);

    auto stream = std::make_shared<Stream>(*this, m_clientId, streamId, flags & RELIABLE_MASK, m_ip, m_port, true);

    m_streams[streamId][true] = stream;
    m_newStreamHandler(stream);
}

void FalconClient::HandlePing(const std::vector<char>& buffer) {
    auto protocolType = static_cast<uint8_t>(ProtocolType::Pong);
    std::vector pongMessage = { static_cast<char>(protocolType) };
    SendTo(m_ip, m_port, std::span(pongMessage.data(), pongMessage.size()));
}

void FalconClient::OnConnectionEvent(std::function<void(bool, UUID)> handler) {
    m_connectionHandler = std::move(handler);
}

void FalconClient::OnDisconnect(std::function<void()> handler) {
    m_disconnectHandler = std::move(handler);
}

void FalconClient::OnCreateStream(std::function<void(std::shared_ptr<Stream>)> handler) {
    m_newStreamHandler = std::move(handler);
}

std::shared_ptr<Stream> FalconClient::CreateStream(bool reliable) {
    auto streamId = m_nextStreamId++;
    auto stream = std::make_shared<Stream>(*this, m_clientId, streamId, reliable, m_ip, m_port);
    m_streams[streamId][false] = stream;

    auto protocolType = static_cast<uint8_t>(ProtocolType::StreamConnect);
    std::vector<char> message = { static_cast<char>(protocolType) };
    message.insert(message.end(), reinterpret_cast<const char*>(&m_clientId), reinterpret_cast<const char*>(&m_clientId) + sizeof(m_clientId));
    uint8_t flags = reliable ? RELIABLE_MASK : 0;message.push_back(flags);
    message.insert(message.end(), reinterpret_cast<const char*>(&streamId), reinterpret_cast<const char*>(&streamId) + sizeof(streamId));
    SendTo(m_ip, m_port, std::span(message.data(), message.size()));

    return stream;
}