#pragma once

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <span>
#include <array>


void hello();

#ifdef WIN32
    using SocketType = unsigned int;
#else
    using SocketType = int;
#endif

class Falcon {
public:
    bool Listen(const std::string& endpoint, uint16_t port);
    bool Connect(const std::string& serverIp, uint16_t port);

    Falcon();
    ~Falcon();
    Falcon(const Falcon&) = default;
    Falcon& operator=(const Falcon&) = default;
    Falcon(Falcon&&) = default;
    Falcon& operator=(Falcon&&) = default;

    int SendTo(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFrom(std::string& from, std::span<char, 65535> message);

    void StartListening();
    static std::pair<std::string /*ip*/, uint16_t /*port*/> GetClientAddress(const std::string &Adress);
    void ListenForMessages();
    std::optional<std::tuple<std::span<char, 65535>, std::string, int>> GetNextMessage();

    std::queue<std::tuple<std::span<char, 65535>, std::string, int>> m_messageQueue;
private:
    int SendToInternal(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFromInternal(std::string& from, std::span<char, 65535> message);

    SocketType m_socket;

};
