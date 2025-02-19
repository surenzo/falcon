#pragma once


#include <span>

class Stream {

	void SendData(std::span<const char> Data);

    void OnDataReceived(std::span<const char> Data);

};
