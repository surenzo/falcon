#pragma once

#include <memory>
#include <string>
#include <span>
#include <functional>
#include <unordered_map>
#include <chrono>

class Stream; // Déclaration de la classe Stream

void hello();

#ifdef WIN32
    using SocketType = unsigned int;
#else
    using SocketType = int;
#endif

class Falcon {
public:
    static std::unique_ptr<Falcon> Listen(const std::string& endpoint, uint16_t port);
    static std::unique_ptr<Falcon> ConnectTo(const std::string& serverIp, uint16_t port);

    void Disconnect();
    
    Falcon();
    ~Falcon();
    Falcon(const Falcon&) = default;
    Falcon& operator=(const Falcon&) = default;
    Falcon(Falcon&&) = default;
    Falcon& operator=(Falcon&&) = default;

    int SendTo(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFrom(std::string& from, std::span<char, 65535> message);

    // Connexion/Déconnexion
    void OnClientConnected(std::function<void(uint64_t)> handler);
    void OnClientDisconnected(std::function<void(uint64_t)> handler);
    void OnConnectionEvent(std::function<void(bool, uint64_t)> handler);
    void OnDisconnect(std::function<void()> handler);

    // Stream
    std::unique_ptr<Stream> CreateStream(uint64_t client, bool reliable); // Server
    std::unique_ptr<Stream> CreateStream(bool reliable); // Client
    void CloseStream(const Stream& stream);

    void SendData(const Stream& stream, std::span<const char> data);
    void OnDataReceived(const Stream& stream, std::span<const char> data);

private:
    int SendToInternal(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFromInternal(std::string& from, std::span<char, 65535> message);

    void TriggerClientConnected(uint64_t clientId);
    void TriggerClientDisconnected(uint64_t clientId);
    void TriggerConnectionEvent(bool success, uint64_t clientId);
    void TriggerDisconnectEvent();

    void HandleTimeout();

    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> m_clients;
    std::function<void(uint64_t)> m_clientConnectedHandler;
    std::function<void(uint64_t)> m_clientDisconnectedHandler;
    std::function<void(bool, uint64_t)> m_connectionEventHandler;
    std::function<void()> m_disconnectHandler;

    SocketType m_socket;
    uint64_t m_nextClientId = 1;
};
