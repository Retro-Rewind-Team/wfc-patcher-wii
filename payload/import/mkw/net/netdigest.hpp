#pragma once

#if RMC

#  include <wwfcBase.hpp>

namespace wwfc::mkw::Net
{

typedef struct {
    u8 _00[0x60 - 0x00];
} NETSHA1CTX;

} // namespace wwfc::mkw::Net

#endif // RMC
