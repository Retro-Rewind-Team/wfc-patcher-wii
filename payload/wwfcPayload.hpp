#pragma once

#include <wwfcTypes.h>

namespace wwfc::Payload
{

extern const wwfc_payload_ex Header;

constinit inline wwfc_boolean_t g_enableAggressivePacketChecks =
    WWFC_BOOLEAN_RESET;
constinit inline wwfc_boolean_t g_enableEventItemIdCheck = WWFC_BOOLEAN_TRUE;
constinit inline wwfc_boolean_t g_enableUltraUncut = WWFC_BOOLEAN_RESET;

} // namespace wwfc::Payload
