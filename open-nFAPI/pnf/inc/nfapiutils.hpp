#pragma once

#include "nfapiutils.h"
#include <string>

inline std::string hexdump(const void *data, size_t data_len)
{
    char buf[512];
    return hexdump(data, data_len, buf, sizeof(buf));
}

inline std::string hexdumpP5(const void *data, size_t data_len)
{
    char buf[512];
    return hexdumpP5(data, data_len, buf, sizeof(buf));
}

inline std::string hexdumpP7(const void *data, size_t data_len)
{
    char buf[512];
    return hexdumpP7(data, data_len, buf, sizeof(buf));
}
