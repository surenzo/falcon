#pragma once

#include <span>
#include <functional>
#include <atomic>

class Stream {
public:
    Stream(uint64_t id, bool reliable);
    
    uint64_t GetId() const;
    bool IsReliable() const;
    
    void SendData(std::span<const char> data);
    void OnDataReceived(std::function<void(std::span<const char>)> handler);

    void Close();
    void HandleLostData();

private:
    uint64_t m_id;
    bool m_reliable;
    std::function<void(std::span<const char>)> m_dataReceivedHandler;
};
