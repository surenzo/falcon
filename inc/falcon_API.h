#pragma once
#include <array>
#include <span>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <Stream.h>
#include <protocol.h>
#include <thread>
typedef uint64_t UUID;

class FalconServer : public Falcon {
    public:
        static std::unique_ptr<FalconServer> ListenTo(uint16_t port);
        std::shared_ptr<Stream> CreateStream(UUID client, bool reliable);

        FalconServer();
        ~FalconServer();
        FalconServer(const FalconServer&) = default;
        FalconServer& operator=(const FalconServer&) = default;
        FalconServer(FalconServer&&) = default;
        FalconServer& operator=(FalconServer&&) = default;


        void Update();

        void OnClientConnected(std::function<void(UUID)> handler);
        void OnClientDisconnected(std::function<void(UUID)> handler);
        void OnCreateStream(std::function<void(std::shared_ptr<Stream>)> handler);
        void CloseStream(const Stream& stream);


    private:
        void HandleConnect( const std::string& ip,const std::vector<char>& buffer);
        void HandleStream(const std::string& from_ip,const std::vector<char>& buffer);
        void HandleStreamConnect(const std::vector<char>& buffer);
        void HandlePing(const std::string& from_ip, const std::vector<char>& buffer);
        void HandleAcknowledgement(const std::string& from_ip, const std::vector<char>& buffer);

        std::unordered_map<std::string, UUID> clients;
        std::unordered_map<UUID, std::chrono::steady_clock::time_point> m_clients_Ping;
        std::unordered_map<UUID, std::chrono::steady_clock::time_point> m_clients_Pong;
        std::unordered_map<uint64_t, std::pair<std::string, uint16_t>> m_clientIdToAddress;

        std::function<void(UUID)> m_clientConnectedHandler;
        std::function<void(UUID)> m_clientDisconnectedHandler;
        std::function<void(std::shared_ptr<Stream>)> m_newStreamHandler;

        std::unordered_map<UUID, std::unordered_map<uint32_t, std::unordered_map<bool, std::shared_ptr<Stream>>>> m_streams;

        uint32_t m_nextStreamId = 1;
    };


class FalconClient : public Falcon {

    public:
        static std::unique_ptr<FalconClient> ConnectTo(const std::string& ip, uint16_t port);
        std::shared_ptr<Stream> CreateStream(bool reliable);

        FalconClient();
        ~FalconClient();
        FalconClient(const FalconClient&) = default;
        FalconClient& operator=(const FalconClient&) = default;
        FalconClient(FalconClient&&) = default;
        FalconClient& operator=(FalconClient&&) = default;


        void Update();

        void OnConnectionEvent(std::function<void(bool, UUID)> handler);
        void OnDisconnect(std::function<void()> handler);
        void OnCreateStream(std::function<void(std::shared_ptr<Stream>)> handler);
    private:
        void HandleConnectMessage(const std::vector<char>& buffer);
        void HandleStreamMessage(const std::vector<char>& buffer);
        void HandleStreamConnect(const std::vector<char>& buffer);
        void HandlePing(const std::vector<char>& buffer);
        void HandleAcknowledgement(const std::string& from_ip, const std::vector<char>& buffer);

        std::function<void(bool, UUID)> m_connectionHandler;
        std::function<void()> m_disconnectHandler;
        std::function<void(std::shared_ptr<Stream>)> m_newStreamHandler;

        std::unordered_map<uint32_t, std::unordered_map<bool,std::shared_ptr<Stream>>> m_streams;

        std::chrono::steady_clock::time_point m_lastPing;
        std::chrono::steady_clock::time_point m_lastPong;

        UUID m_clientId;
        std::string m_ip;
        uint16_t m_port;
        uint32_t m_nextStreamId = 1;
    };
    