#pragma once

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <span>
#include <array>
#include <mutex> 


void hello();

#ifdef WIN32
    using SocketType = unsigned int;
#else
    using SocketType = int;
#endif
struct Message {
    std::vector<char> data;
    std::string from_ip;
    int recv_size;

    Message(const std::vector<char>& data, const std::string& from_ip, int recv_size)
        : data(data), from_ip(from_ip), recv_size(recv_size) {}
};
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
    std::optional<Message> GetNextMessage();

    std::mutex m_queueMutex;
    std::queue<Message> m_messageQueue;
private:
    int SendToInternal(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFromInternal(std::string& from, std::span<char, 65535> message);

    SocketType m_socket;

};
