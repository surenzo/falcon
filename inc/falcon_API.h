#pragma once


#include "falcon.h"
#include "Stream.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>


typedef uint64_t UUID;

class FalconServer : Falcon {

public:
    std::unique_ptr<Stream> CreateStream(UUID client, bool reliable);

    FalconServer();
    ~FalconServer();
    FalconServer(const FalconServer&) = default;
    FalconServer& operator=(const FalconServer&) = default;
    FalconServer(FalconServer&&) = default;
    FalconServer& operator=(FalconServer&&) = default;


    void Listen(uint16_t port);
    void OnClientConnected(std::function<void(UUID)> handler);
    void OnClientDisconnected(std::function<void(UUID)> handler);
    void CloseStream(const Stream& stream);

private:
    std::unordered_map<std::string, UUID> clients;
    std::unordered_map<UUID, std::chrono::steady_clock::time_point> m_clients;
    std::function<void(UUID)> m_clientConnectedHandler;
    std::function<void(UUID)> m_clientDisconnectedHandler;
    std::unordered_map<uint64_t, std::unique_ptr<Stream>> m_streams;


};

class FalconClient : Falcon {

public:
    FalconClient();
    ~FalconClient();
    FalconClient(const FalconClient&) = default;
    FalconClient& operator=(const FalconClient&) = default;
    FalconClient(FalconClient&&) = default;
    FalconClient& operator=(FalconClient&&) = default;

	void ConnectTo(const std::string& ip, uint16_t port);

	void OnConnectionEvent(std::function<void(bool, UUID)> handler);

    void OnDisconnect(std::function<void()> handler);

    std::unique_ptr<Stream> CreateStream(bool reliable);

private:
    std::function<void(bool, UUID)> m_connectionHandler;
    std::function<void()> m_disconnectHandler;
    std::unordered_map<uint64_t, std::unique_ptr<Stream>> m_streams;
    UUID m_clientId;
};
