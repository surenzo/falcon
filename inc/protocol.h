// protocol.h
#pragma once

#include <cstdint>

enum class ProtocolType : uint8_t {
    Connect,
    Stream,
    Ping,
    Pong,
    StreamConnect,
    Acknowledgement,
};