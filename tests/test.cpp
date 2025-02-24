#include <string>
#include <array>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "falcon.h"

TEST_CASE("Can Listen", "[falcon]") {
    Falcon receiver;
    bool IsAlive =  receiver.Listen("127.0.0.1", 5555);
    REQUIRE(IsAlive);
}

TEST_CASE("Can Connect", "[falcon]") {
    Falcon sender;
    bool IsAlive = sender.Connect("127.0.0.1", 5556);
    REQUIRE(IsAlive);
}

TEST_CASE("Can Send To", "[falcon]") {
    Falcon sender;
    sender.Connect("127.0.0.1", 5556);
    Falcon receiver;
    receiver.Listen("127.0.0.1", 5555);
    std::string message = "Hello World!";
    std::span data(message.data(), message.size());
    int bytes_sent = sender.SendTo("127.0.0.1", 5555, data);
    REQUIRE(bytes_sent == message.size());
}

TEST_CASE("Can Receive From", "[falcon]") {
    Falcon sender;
    sender.Connect("127.0.0.1", 5556);
    Falcon receiver;
    receiver.Listen("127.0.0.1", 5555);
    std::string message = "Hello World!";
    std::span data(message.data(), message.size());
    int bytes_sent = sender.SendTo("127.0.0.1", 5555, data);
    REQUIRE(bytes_sent == message.size());
    std::string from_ip;
    std::array<char, 65535> buffer;
    int bytes_received = receiver.ReceiveFrom(from_ip, buffer);

    REQUIRE(bytes_received == message.size());
    REQUIRE(std::equal(buffer.begin(), buffer.begin() + bytes_received, message.begin(), message.end()));
    REQUIRE(from_ip == "127.0.0.1:5556");
}