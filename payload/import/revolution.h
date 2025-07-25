#pragma once

#include <wwfcUtil.h>

#ifdef __cplusplus
extern "C" {

namespace RVL
{
#endif

typedef struct SOInAddr {
    u32 addr;
} SOInAddr;

typedef struct SOSockAddrIn {
    u8 len;
    u8 family;
    u16 port;
    SOInAddr addr;
} SOSockAddrIn;

static_assert(sizeof(SOInAddr) == 4);
static_assert(sizeof(SOSockAddrIn) == 8);

typedef enum {
    SCLanguageJapanese = 0x00,
    SCLanguageEnglish = 0x01,
    SCLanguageGerman = 0x02,
    SCLanguageFrench = 0x03,
    SCLanguageSpanish = 0x04,
    SCLanguageItalian = 0x05,
    SCLanguageDutch = 0x06,
    SCLanguageSimplifiedChinese = 0x07,
    SCLanguageTraditionalChinese = 0x08,
    SCLanguageKorean = 0x09,

    // Server Languages

    // Custom Languages
    SCLanguageCzech = 0x0A,
    SCLanguageNorwegian = 0x0B,
    SCLanguageRussian = 0x0C,
    SCLanguagePortuguese = 0x0D,
    SCLanguageArabic = 0x0E,
    SCLanguageTurkish = 0x10,
    SCLanguageFinnish = 0x11,

    SCLanguageEnglishEU = 0x12,
    SCLanguageFrenchEU = 0x13,
    SCLanguageSpanishEU = 0x14,

    // Custom Lanuages
    SCLanguagePortugueseEU = 0x15,

    SCLanguageCount = 0x16,
} SCLanguage;

LONGCALL bool OSDisableInterrupts( //
    void
) AT(ADDRESS_OSDisableInterrupts);

LONGCALL void OSRestoreInterrupts( //
    bool enabled
) AT(ADDRESS_OSRestoreInterrupts);

LONGCALL void OSReport( //
    const char* format, ...
) AT(ADDRESS_OSReport);

LONGCALL void DCFlushRange( //
    void* ptr, u32 size
) AT(ADDRESS_DCFlushRange);

LONGCALL void ICInvalidateRange( //
    void* ptr, u32 size
) AT(ADDRESS_ICInvalidateRange);

typedef struct {
    void* data;
    u32 size;
} IOVector;

LONGCALL s32 IOS_Open( //
    const char* path, u32 flags
) AT(ADDRESS_IOS_Open);

LONGCALL s32 IOS_Close( //
    s32 fd
) AT(ADDRESS_IOS_Close);

LONGCALL s32 IOS_Ioctlv( //
    s32 fd, u32 cmd, u32 in_count, u32 out_count, IOVector* vec
) AT(ADDRESS_IOS_Ioctlv);

#if RMC

LONGCALL u8 SCGetLanguage( //
    void
) AT(RMCXD_PORT(0x801B1D0C, 0x801B1C6C, 0x801B1C2C, 0x801B2068));

#endif // RMC

LONGCALL bool SCGetProductSN( //
    u32* serial
) AT(ADDRESS_SCGetProductSN);

#ifdef __cplusplus
}
}
#endif
