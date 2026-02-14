#pragma once

#include <iostream>
#include <cstdint>
#include <string.h>

#include <switch.h>

#include <sstream>
#include <format>

#define MAX_PROFILES 256

namespace ipglobal {
    typedef uint32_t ip_addr;
    typedef struct {
        uint8_t p1: 8;
        uint8_t p2: 8;
        uint8_t p3: 8;
        uint8_t p4: 8;
        uint8_t p5: 8;
        uint8_t p6: 8;
    } mac_addr;

    mac_addr getMacAddress();
    ip_addr getIPAddress();

    std::string getIPString(ip_addr addr);
    std::string getMacString(mac_addr addr);
    std::string getPartialMacString(mac_addr addr);

    Result cleanup();
}