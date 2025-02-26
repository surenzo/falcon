#include <iostream>
#include <protocol.h>
#include <thread>
#include "../inc/Stream.h"
#include "spdlog/details/os.h"

Stream::Stream(Falcon &falcon, uint64_t clientId, uint32_t streamId, bool reliable, const std::string &ip, uint16_t port, bool isServer) :
    m_falcon(falcon), m_clientId(clientId), m_streamId(streamId), m_reliable(reliable), m_ip(ip), m_port(port), m_isServer(isServer), m_dataReceivedHandler() {
}

Stream::~Stream() {
}

void Stream::Update() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = m_packetMap.begin(); it != m_packetMap.end(); ++it) {
        auto& [lastSentTime, packet] = it->second;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSentTime).count() > 100) {
            m_falcon.SendTo(m_ip, m_port, std::span(packet.data(), packet.size()));
            lastSentTime = now;
        }
    }
}

void Stream::SendData(std::span<const char> data) {
    std::vector<char> packet;
    auto protocolType = static_cast<uint8_t>(ProtocolType::Stream);
    packet.push_back(protocolType);
    packet.insert(packet.end(), reinterpret_cast<const char*>(&m_clientId), reinterpret_cast<const char*>(&m_clientId) + sizeof(m_clientId));
    packet.insert(packet.end(), reinterpret_cast<const char*>(&m_streamId), reinterpret_cast<const char*>(&m_streamId) + sizeof(m_streamId));
    uint8_t flags = 0;
    if (m_reliable) flags |= RELIABLE_MASK;
    if (m_isServer) flags |= CLIENT_STREAM_MASK;
    packet.push_back(flags);
    packet.insert(packet.end(), reinterpret_cast<const char*>(&m_nextPacketId), reinterpret_cast<const char*>(&m_nextPacketId) + sizeof(m_nextPacketId));
    packet.insert(packet.end(), data.begin(), data.end());

    if (m_reliable) {
        m_packetMap[m_nextPacketId] = std::make_pair(std::chrono::steady_clock::now(), packet);
        m_falcon.SendTo(m_ip, m_port, packet);
    } else {
        m_falcon.SendTo(m_ip, m_port, packet);
    }
    m_nextPacketId++;
}

void Stream::OnDataReceived(std::span<const char> data) {
    std::vector<char> packet(data.begin(), data.end());
    if (m_reliable) {
        std::vector<char> ackPacket;
        auto protocolType = static_cast<uint8_t>(ProtocolType::Acknowledgement);
        auto packetId = *reinterpret_cast<const uint32_t*>(&packet[14]);
        ackPacket.push_back(protocolType);
        ackPacket.insert(ackPacket.end(), reinterpret_cast<const char*>(&m_clientId), reinterpret_cast<const char*>(&m_clientId) + sizeof(m_clientId));
        ackPacket.insert(ackPacket.end(), reinterpret_cast<const char*>(&m_streamId), reinterpret_cast<const char*>(&m_streamId) + sizeof(m_streamId));
        uint8_t flags = RELIABLE_MASK;
        if (m_isServer) flags |= CLIENT_STREAM_MASK;
        ackPacket.push_back(flags);
        ackPacket.insert(ackPacket.end(), reinterpret_cast<const char*>(&packetId), reinterpret_cast<const char*>(&packetId) + sizeof(packetId));
        m_falcon.SendTo(m_ip, m_port, ackPacket);
    }

    std::vector<char> trueData(data.begin() + 18, data.end());
    if (m_dataReceivedHandler) {
        m_dataReceivedHandler(trueData);
    } else {
        std::cerr << "Error: Data received handler not set" << std::endl;
    }
}

void Stream::Acknowledge(uint32_t packetId) {
    m_packetMap.erase(packetId);
}

void Stream::OnDataReceivedHandler(std::function<void(std::span<const char>)> handler) {
    m_dataReceivedHandler = std::move(handler);
}