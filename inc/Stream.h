#pragma once


#include <cstdint>
#include <functional>
#include <span>

#include "falcon.h"

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

	uint64_t GetClientId() const { return m_clientId; }
	uint32_t GetStreamId() const { return m_streamId; }

private:
	std::string m_ip;
	uint16_t m_port;
	Falcon& m_falcon;
	uint64_t m_clientId;
	uint32_t m_streamId;
	bool m_reliable;
	std::function<void(std::span<const char>)> m_dataReceivedHandler;
};
