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

// Gestion des streams
std::unique_ptr<Stream> Falcon::CreateStream(uint64_t client, bool reliable) {
    uint64_t streamId = m_nextClientId++;
    return std::make_unique<Stream>(streamId, reliable);
}

std::unique_ptr<Stream> Falcon::CreateStream(bool reliable) {
    uint64_t streamId = m_nextClientId++;
    return std::make_unique<Stream>(streamId, reliable);
}

void Falcon::CloseStream(const Stream& stream) {
    // Logique pour fermer un stream
}

void Falcon::SendData(const Stream& stream, std::span<const char> data) {
    // Logic to send data using the stream
}

void Falcon::OnDataReceived(const Stream& stream, std::span<const char> data) {
    // Handle received data for the stream
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
