#pragma once


#include <cstdint>
#include <functional>
#include <span>

#include "falcon.h"
constexpr char CLIENT_STREAM_MASK = 1<<7 ; //1000 0000
constexpr char RELIABLE_MASK = 1<<6 ; //0100 0000
class Stream {

public:

	Stream(Falcon& falcon, uint64_t clientId, uint32_t streamId, bool reliable, const std::string& ip, uint16_t port);
	~Stream();
	Stream(const Stream&) = default;
	Stream& operator=(const Stream&) = default;
	Stream(Stream&&) = default;
	Stream& operator=(Stream&&) = default;


	void SendData(std::span<const char> Data);
    void OnDataReceived(std::span<const char> Data);

	void SetDataReceivedHandler(std::function<void(std::span<const char>)> handler);


	uint64_t GetClientId() const { return m_clientId; }
	uint32_t GetStreamId() const { return m_streamId; }

private:
	std::string m_ip;
	uint16_t m_port;
	Falcon& m_falcon;
	uint64_t m_clientId;
	uint32_t m_streamId;
	bool m_reliable;
	bool m_isServer;
	std::function<void(std::span<const char>)> m_dataReceivedHandler;
	std::vector<std::span<const char>> m_allPackets;
	uint32_t m_nextPacketId = 0;
};
