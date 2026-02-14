#include "ipglobal.hpp"

ipglobal::mac_addr ipglobal::getMacAddress() {
    SetCalMacAddress* mac_in = new SetCalMacAddress();
    mac_addr mac_out;

    Result get_res = setcalGetWirelessLanMacAddress(mac_in);

    if (!R_SUCCEEDED(get_res)) {
        return mac_out;
    }

    mac_out.p1 = mac_in->addr[0];
    mac_out.p2 = mac_in->addr[1];
    mac_out.p3 = mac_in->addr[2];
    mac_out.p4 = mac_in->addr[3];
    mac_out.p5 = mac_in->addr[4];
    mac_out.p6 = mac_in->addr[5];

    delete mac_in;
    return mac_out;
}

ipglobal::ip_addr ipglobal::getIPAddress() {
    ipglobal::ip_addr address;
    nifmGetCurrentIpAddress(&address);

    // swap endianness
    ipglobal::ip_addr swapped = address;
    
    u8* optr = (u8*) &address;
    u8* sptr = (u8*) &swapped;
    
    sptr[0] = optr[3];
    sptr[1] = optr[2];
    sptr[2] = optr[1];
    sptr[3] = optr[0];

    return swapped;
}

std::string ipglobal::getIPString(ipglobal::ip_addr addr) {
    std::string out;

    out += std::to_string((uint8_t)((addr >> 24) & 0xff));
    out += ".";

    out += std::to_string((uint8_t)((addr >> 16) & 0xff));
    out += ".";

    out += std::to_string((uint8_t)((addr >> 8) & 0xff));
    out += ".";

    out += std::to_string((uint8_t)(addr & 0xff));

    return out;
}

std::string ipglobal::getMacString(ipglobal::mac_addr addr) {
    return std::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
        (uint8_t)addr.p1, (uint8_t)addr.p2, (uint8_t)addr.p3,
        (uint8_t)addr.p4, (uint8_t)addr.p5, (uint8_t)addr.p6);
}

std::string ipglobal::getPartialMacString(ipglobal::mac_addr addr) {
    return std::format("{:02x}:{:02x}", (uint8_t)addr.p5, (uint8_t)addr.p6);
}

Result ipglobal::cleanup() {
    SetSysNetworkSettings settings[MAX_PROFILES];
    s32 total = 0;

    Result get = setsysGetNetworkSettings(&total, settings, MAX_PROFILES);
    if (R_FAILED(get)) {
        return get;
    }

    SetSysNetworkSettings filtered[total];
    s32 filteredTotal = 0;

    for (int i = 0; i < total; i++) {
        if (settings[i].access_point_ssid_len > 0 && !settings[i].wired_flag) {
            memcpy(&(filtered[filteredTotal]), &(settings[i]), sizeof(SetSysNetworkSettings));
            filteredTotal++;
        }
    }

    Result set = setsysSetNetworkSettings(filtered, filteredTotal);
    if (R_FAILED(set)) {
        return set;
    }

    return 0;
}