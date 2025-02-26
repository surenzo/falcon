#include <algorithm>
#include <iostream>
#include <optional>
#include <thread>

#include "falcon.h"

int Falcon::SendTo(const std::string &to, uint16_t port, const std::span<const char> message)
{
    return SendToInternal(to, port, message);
}

int Falcon::ReceiveFrom(std::string& from, const std::span<char, 65535> message)
{
    return ReceiveFromInternal(from, message);
}

void Falcon::StartListening() {
    std::thread listenerThread(&Falcon::ListenForMessages, this);
    listenerThread.detach();
}

void Falcon::ListenForMessages() {
    while (true) {
        std::string from_ip;
        from_ip.resize(255);
        std::array<char, 65535> buffer;
        int recv_size = ReceiveFrom(from_ip, buffer);
        if (recv_size <= 0) return;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            std::vector<char> message_data(buffer.data(), buffer.data() + recv_size);
            m_messageQueue.emplace(message_data, from_ip, recv_size);
        }
    }
}
std::optional<Message> Falcon::GetNextMessage() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_messageQueue.empty()) {
        return std::nullopt;
    }
    auto message = m_messageQueue.front();
    m_messageQueue.pop();
    return message;
}
std::pair<std::string, uint16_t> Falcon::GetClientAddress(const std::string &Address) {
    std::string new_ip ;
    uint16_t new_port = 0;
    if (auto pos = Address.find_last_of(':'); pos != std::string::npos) {
        new_ip = Address.substr(0, pos);
        const std::string port_str = Address.substr(++pos);
        new_port = atoi(port_str.c_str());
    }
    return std::make_pair(new_ip, new_port);
}