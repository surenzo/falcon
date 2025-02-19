//
// Created by super on 18/02/2025.
//

#include "../inc/Stream.h"

Stream::Stream(Falcon& falcon, uint64_t clientId, uint32_t streamId, bool reliable, const std::string& ip, uint16_t port)
    : m_falcon(falcon), m_clientId(clientId), m_streamId(streamId), m_reliable(reliable), m_ip(ip), m_port(port) {}

Stream::~Stream() {

}

void Stream::SendData(std::span<const char> data) {

    std::vector<char> packet;
    packet.push_back(0x01);
    packet.insert(packet.end(), reinterpret_cast<const char*>(&m_streamId),
                  reinterpret_cast<const char*>(&m_streamId) + sizeof(m_streamId));
    packet.insert(packet.end(), data.begin(), data.end());

    m_falcon.SendTo(m_ip, m_port, packet);
}

void Stream::OnDataReceived(std::span<const char> Data) {

}