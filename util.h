#pragma once

#include <vector>
#include <bits/stdint-uintn.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<uint8_t>& PrepareTcpData(std::vector<uint8_t> &data, uint8_t tcp_begin = 0xF1, uint8_t tcp_end = 0xF4);
uint8_t GetCRC8(std::vector<uint8_t> v);
uint16_t GetCRC16(std::vector<uint8_t> v, int start_index = 0);

/*#include <chrono>
#include <ctime>
#include <vector>
#include <string>
#include <linux/can.h>

namespace chrono = std::chrono;

///////////////////////////////////////////////////////////////////////////////////////////////////
timeval to_timeval(chrono::system_clock::time_point tp);
chrono::system_clock::time_point to_time_point(timeval tv);

std::string to_string(chrono::system_clock::time_point tp);
std::string to_string(const char* format, chrono::system_clock::time_point tp);
std::string to_string(time_t ts);*/
