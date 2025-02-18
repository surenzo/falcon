#include "falcon.h"
#include <span>
#include <iostream>

int Falcon::SendTo(const std::string &to, uint16_t port, const std::span<const char> message)
{
    return SendToInternal(to, port, message);
}

int Falcon::ReceiveFrom(std::string& from, const std::span<char, 65535> message)
{
    return ReceiveFromInternal(from, message);
}

void Falcon::OnClientConnected(std::function<void(uint64_t)> handler)
{
    m_clientConnectedHandler = handler;
}

void Falcon::OnClientDisconnected(std::function<void(uint64_t)> handler)
{
    m_clientDisconnectedHandler = handler;
}

void Falcon::OnConnectionEvent(std::function<void(bool, uint64_t)> handler)
{
    m_connectionEventHandler = handler;
}

void Falcon::OnDisconnect(std::function<void()> handler)
{
    m_disconnectHandler = handler;
}

// Gestion des clients et des connexions
void Falcon::TriggerClientConnected(uint64_t clientId)
{
    if (m_clientConnectedHandler) {
        m_clientConnectedHandler(clientId);
    }
}

void Falcon::TriggerClientDisconnected(uint64_t clientId)
{
    if (m_clientDisconnectedHandler) {
        m_clientDisconnectedHandler(clientId);
    }
}

void Falcon::TriggerConnectionEvent(bool success, uint64_t clientId)
{
    if (m_connectionEventHandler) {
        m_connectionEventHandler(success, clientId);
    }
}

void Falcon::TriggerDisconnectEvent()
{
    if (m_disconnectHandler) {
        m_disconnectHandler();
    }
}



void Falcon::HandleTimeout()
{
    auto now = std::chrono::steady_clock::now();
    for (auto it = m_clients.begin(); it != m_clients.end();) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
        if (duration.count() > 1) {
            TriggerClientDisconnected(it->first);
            it = m_clients.erase(it);
        } else {
            ++it;
        }
    }
}
