//
// Created by super on 18/02/2025.
//

#include <protocol.h>

#include "../inc/Stream.h"

Stream::Stream(Falcon& falcon, uint64_t clientId, uint32_t streamId, bool reliable, const std::string& ip, uint16_t port)
    : m_falcon(falcon), m_clientId(clientId), m_streamId(streamId), m_reliable(reliable), m_ip(ip), m_port(port) {}

Stream::~Stream() {

}

void Stream::SendData(std::span<const char> data) {
    std::vector<char> packet;
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Stream);
    packet.push_back(protocolType);
    packet.push_back(m_clientId);
    packet.push_back(m_streamId);
    uint8_t flags = 0;
    if (m_reliable)
        flags |= RELIABLE_MASK;
    if (m_isServer)
        flags |= CLIENT_STREAM_MASK;
    packet.push_back(flags);
    packet.push_back(m_nextPacketId);
    packet.insert(packet.end(), data.begin(), data.end());

    if (m_reliable) {
        m_allPackets.push_back(packet);
    }
    m_falcon.SendTo(m_ip, m_port, packet);
    m_nextPacketId++;
}

void Stream::OnDataReceived(std::span<const char> Data) {
    std::vector<char> packet = {Data.begin(), Data.end()};
    uint64_t clientId = packet[1];
    uint32_t streamId = packet[9];
    uint8_t flags = packet[10];
    if (flags & RELIABLE_MASK) {
        //renvoyer un ack avec le numéro du paket
    }
    if (flags & CLIENT_STREAM_MASK) {
        //envoyer un ack avec le numéro du paket
    }
)

}

void Stream::SetDataReceivedHandler(std::function<void(std::span<const char>)> handler) {
    m_dataReceivedHandler = std::move(handler);
}