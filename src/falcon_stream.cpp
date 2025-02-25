//
// Created by super on 18/02/2025.
//

#include <iostream>
#include <protocol.h>
#include <thread>

#include "../inc/Stream.h"


Stream::Stream(Falcon &falcon, uint64_t clientId, uint32_t streamId, bool reliable, const std::string &ip, uint16_t port, bool isServer) :
m_falcon(falcon), m_clientId(clientId), m_streamId(streamId), m_reliable(reliable), m_ip(ip), m_port(port), m_isServer(isServer), m_dataReceivedHandler() {

}

Stream::~Stream() {

}

void Stream::SendData(std::span<const char> data) {
    std::vector<char> packet;
    if (data.size() > packet.max_size() - 15) {
        throw std::length_error("Data size exceeds maximum allowable size for std::vector");
    }
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Stream);
    packet.insert(packet.end(), protocolType);
    packet.insert(packet.end(), reinterpret_cast<const char*>(&m_clientId), reinterpret_cast<const char*>(&m_clientId) + sizeof(m_clientId));
    packet.insert(packet.end(), reinterpret_cast<const char*>(&m_streamId), reinterpret_cast<const char*>(&m_streamId) + sizeof(m_streamId));
    uint8_t flags = 0;
    if (m_reliable)
        flags |= RELIABLE_MASK;
    if (m_isServer)
        flags |= CLIENT_STREAM_MASK;
    packet.insert(packet.end(), flags);
    packet.insert(packet.end(), m_nextPacketId);
    packet.insert(packet.end(), data.begin(), data.end());

    if (m_reliable) {
        m_allPackets.push_back(packet);
        m_ackReceived[m_nextPacketId] = false;
        SendWithRetry(packet, m_nextPacketId);
    }  else {
        m_falcon.SendTo(m_ip, m_port, packet);
    }
    m_nextPacketId++;
}

void Stream::OnDataReceived(std::span<const char> Data) {
    if (m_reliable) {
        std::vector<char> packet;
        uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Acknowledgement);
        uint8_t packetId = Data[11];
        packet.insert(packet.end(), protocolType);
        packet.insert(packet.end(), reinterpret_cast<const char*>(&m_clientId), reinterpret_cast<const char*>(&m_clientId) + sizeof(m_clientId));
        packet.insert(packet.end(), reinterpret_cast<const char*>(&m_streamId), reinterpret_cast<const char*>(&m_streamId) + sizeof(m_streamId));
        packet.insert(packet.end(), packetId);
        m_falcon.SendTo(m_ip, m_port, packet);
    }
    std::vector<char> packet = {Data.begin(), Data.end()};
    uint8_t flags = packet[10];
    if (flags & RELIABLE_MASK) {
        // Send an ack with the packet number
    }
    // Once you have everything, verify and write the data to m_data
    std::vector<char> trueData = {Data.begin() + 15, Data.end()};
    // If it's the last packet, call OnDataReceived

    if (m_dataReceivedHandler) {
        m_dataReceivedHandler(trueData);
    } else {
        std::cerr << "Error: Data received handler not set" << std::endl;
    }
}

void Stream::Acknowledge(uint8_t packetId) {
    m_ackReceived[packetId] = true;
}

void Stream::SendWithRetry(const std::vector<char>& data,uint8_t packetId) { // Make this a member function
    m_falcon.SendTo(m_ip, m_port, std::span(data.data(), data.size()));
    // Schedule a retry if no acknowledgement is received
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!m_ackReceived[packetId]) {
        SendWithRetry(data, packetId);
    } else {
        m_ackReceived.erase(packetId);
    }
}

void Stream::OnDataReceivedHandler(std::function<void(std::span<const char>)> handler) {
    m_dataReceivedHandler = std::move(handler);
}