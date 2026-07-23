#include "import/revolution.h"
#include "wwfcBase.hpp"
#include "wwfcLibC.hpp"
#include "wwfcLog.hpp"
#include "wwfcPatch.hpp"
#include "wwfcTypes.h"

namespace wwfc::Support
{

static_assert(sizeof(WWFC_DOMAIN) <= sizeof("nintendowifi.net"));

// Unpatch the auth response hook
WWFC_DEFINE_PATCH = Patch::Write(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_AUTH_HANDLERESP_HOOK, //
    (u32[]) {AUTH_HANDLERESP_UNPATCH}
);

static const char* FixHostname(const char* name, char* out)
{
    if (std::strlen(name) > 256 - sizeof(WWFC_DOMAIN)) {
        // Hostname is too big to replace
        return name;
    }

    for (u32 i = 0; name[i] != '\0'; i++) {
        auto replaceDomain = [&](const char* check, u32 checkSize,
                                 const char* replace, u32 replaceSize) {
            if (std::strncmp(name + i, check, checkSize) != 0) {
                return false;
            }

            // Replace with the custom hostname
            std::memcpy(out, name, i);
            std::memcpy(out + i, replace, replaceSize);
            std::strcpy(out + i + replaceSize, name + i + checkSize);
            WWFC_LOG_INFO_FMT("Hostname: %s -> %s", name, out);
            return true;
        };

        // Replace nintendowifi.net with the custom name
        if (replaceDomain(
                "nintendowifi.net", sizeof("nintendowifi.net") - 1, WWFC_DOMAIN,
                sizeof(WWFC_DOMAIN) - 1
            )) {
            return out;
        }

        // Redirect gamespy.com to the "gs" subdomain, required for some games
        if (replaceDomain(
                "gamespy.com", sizeof("gamespy.com"), "gs." WWFC_DOMAIN,
                sizeof("gs." WWFC_DOMAIN) - 1
            )) {
            return out;
        }

        // WWFC patch over an already Wiimmfi patched game
        if (replaceDomain(
                "wiimmfi.de", sizeof("wiimmfi.de") - 1, WWFC_DOMAIN,
                sizeof(WWFC_DOMAIN) - 1
            )) {
            return out;
        }
    }

    // No custom hostname
    return name;
}

WWFC_DEFINE_CTR_STUB( //
    ADDRESS_gethostbyname + 0x10, //
    void* gethostbyname(const char* name),

    // clang-format off
    stwu    r1, -0x30(r1);
    mflr    r0;
    stw     r0, 0x34(r1);
    addi    r11, r1, 0x30;
    // clang-format on
)

WWFC_DEFINE_PATCH = Patch::BranchWithCTR(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_gethostbyname, //
    [](char* name) -> void* {
    char fixedName[256];
    return gethostbyname(FixHostname(name, fixedName));
}
);

WWFC_DEFINE_CTR_STUB( //
    ADDRESS_SOInetAtoN + 0x10, //
    u32 SOInetAtoN(const char* name, u8* param_2),

    // clang-format off
    stwu    r1, -0x30(r1);
    mflr    r0;
    stw     r0, 0x34(r1);
    addi    r11, r1, 0x30;
    // clang-format on
)

WWFC_DEFINE_PATCH = Patch::BranchWithCTR(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_SOInetAtoN, //
    [](const char* name, u8* param_2) -> u32 {
    char fixedName[256];
    return SOInetAtoN(FixHostname(name, fixedName), param_2);
}
);

WWFC_DEFINE_CTR_STUB( //
    ADDRESS_SOGetAddrInfo + 0x10, //
    u32 SOGetAddrInfo(const char* name, u32 param_2, u32 param_3, u32 param_4),

    // clang-format off
    stwu    r1, -0x40(r1);
    mflr    r0;
    stw     r0, 0x44(r1);
    addi    r11, r1, 0x40;
    // clang-format on
)

WWFC_DEFINE_PATCH = Patch::BranchWithCTR(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_SOGetAddrInfo, //
    [](const char* name, u32 param_2, u32 param_3, u32 param_4) -> u32 {
    char fixedName[256];
    return SOGetAddrInfo(
        FixHostname(name, fixedName), param_2, param_3, param_4
    );
}
);

// Disable SSL in NHTTP

WWFC_DEFINE_PATCH = Patch::WriteASM(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_NHTTP_HTTPS_PORT_PATCH, //
    1, ASM_LAMBDA((), li r0, 80)
);
WWFC_DEFINE_PATCH = Patch::WriteASM(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_NHTTPi_SocSSLConnect, //
    2, ASM_LAMBDA((), li r3, 0; blr)
);

#if ADDRESS_GHIPARSEURL_HTTPS_PATCH

// Disable SSL in GameSpy HTTP
WWFC_DEFINE_PATCH = Patch::WriteASM(
    WWFC_PATCH_LEVEL_SUPPORT, //
    ADDRESS_GHIPARSEURL_HTTPS_PATCH, //
    1, ASM_LAMBDA((), li r0, 0)
);

#endif

void ChangeAuthURL()
{
    *reinterpret_cast<const char**>(ADDRESS_NASWII_AC_URL_POINTER) =
        "http://nas." WWFC_DOMAIN "/ac";
}

#if RMC

// Remove duplicate Host header from DWC Login
WWFC_DEFINE_PATCH = Patch::Write(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x800ed868, 0x800ed7c8, 0x800ed788, 0x800ed8e0, DEMOTODO),
    (u32[]) {0x60000000}
);

u32 NHTTPi_FixHostHeader(void* request, const char* data, int length)
{
    [[gnu::longcall]] u32 NHTTPi_SendData( //
        void *request, const void *data, int length
    ) AT(RMCXD_PORT(0x801d6384, 0x801d62e4, 0x801d62a4, 0x801d66e0, DEMOTODO));

    char dataNullTerminated[256];
    int newLength = std::min(length + 1, 256);
    std::memcpy(dataNullTerminated, data, newLength - 1);
    dataNullTerminated[newLength - 1] = '\0';

    WWFC_LOG_INFO_FMT(
        "NHTTPi_FixHostHeader: Recieved '%s':%d", dataNullTerminated, length
    );

    char fixedData[256];
    const char* out = FixHostname(dataNullTerminated, fixedData);

    WWFC_LOG_INFO_FMT("NHTTPi_FixHostHeader: Altered data to '%s'", out);

    return NHTTPi_SendData(request, out, std::strlen(out));
}

// Replace NHTTPi_ThreadSendProc's Host header with the correct domain
// Intercepts its call to NHTTPi_SendData to swap out its hostname
WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x801d7a34, 0x801d7994, 0x801d7954, 0x801d7d90, DEMOTODO),
    NHTTPi_FixHostHeader
);

const char* sakeFixURL(const char* src, char* dest)
{
    const char* out = FixHostname(src, dest);

    int len = std::strlen(out);
    if (len >= 0x80) {
        WWFC_LOG_ERROR_FMT(
            "Replaced sake URL is too long (%s -> %s:%d)! This may cause "
            "corruption",
            src, out, len
        );
    }

    return out;
}

u32 SakeInterceptSetFileUploadURL(char* dest, const char* src)
{
    [[gnu::longcall]] u32 sakeSetFileUploadURL( //
        char *dest, const char *src
    ) AT(RMCXD_PORT(0x80122328, 0x80122288, 0x80122248, 0x801223a0, DEMOTODO));

    char fixedName[256];
    const char* out = sakeFixURL(src, fixedName);

    int ret = sakeSetFileUploadURL(dest, out);

    char* set = reinterpret_cast<char*>(
        RMCXD_PORT(0x802f3fc0, 0x802efc40, 0x802f3940, 0x802e1fc0, DEMOTODO)
    );
    WWFC_LOG_INFO_FMT("Set sakeFileUploadURL to %s", set);

    return ret;
}

u32 SakeInterceptSetFileDownloadURL(char* dest, char* src)
{
    [[gnu::longcall]] u32 sakeSetFileDownloadURL( //
char *dest, const char *src
) AT(RMCXD_PORT(0x80122218, 0x80122178, 0x80122138, 0x80122290, DEMOTODO));

    char fixedName[256];
    const char* out = sakeFixURL(src, fixedName);

    int ret = sakeSetFileDownloadURL(dest, out);

    char* set = reinterpret_cast<char*>(
        RMCXD_PORT(0x802f3f40, 0x802efbc0, 0x802f38c0, 0x802e1f40, DEMOTODO)
    );
    WWFC_LOG_INFO_FMT("Set sakeFileDownloadURL to %s", set);

    return ret;
}

// Intercept SakeFileServer upload/download
WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x800ea484, 0x800ea3e4, 0x800ea3a4, 0x800ea4fc, DEMOTODO),
    SakeInterceptSetFileUploadURL
);

WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x800ea490, 0x800ea3f0, 0x800ea3b0, 0x800ea508, DEMOTODO),
    SakeInterceptSetFileDownloadURL
);

[[gnu::longcall]] u32 snprintf( //
    char *str, int size, const char *format, ...
) AT(RMCXD_PORT(0x80011938, 0x80010dd8, 0x8001185c, 0x800119a0, DEMOTODO));

u32 SakeInterceptSetStorageServerURL(
    char* dest, int size, char* format, char* gameName
)
{
    char fixedFormt[256];
    const char* out = FixHostname(format, fixedFormt);

    WWFC_LOG_INFO_FMT("Set sake StorageServer URL base to %s", out);

    return snprintf(dest, size, out, gameName);
}

WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x800ea40c, 0x800ea36c, 0x800ea32c, 0x800ea46c, DEMOTODO),
    SakeInterceptSetStorageServerURL
);

#  define SakeInterceptGhostDownloadURLBody(...)                               \
      char fixedFormat[256];                                                   \
      const char* out = FixHostname(format, fixedFormat);                      \
                                                                               \
      WWFC_LOG_INFO_FMT("Replace sake GhostDownloadURL base to %s", out);      \
                                                                               \
      return snprintf(__VA_ARGS__)

u32 SakeInterceptGhostDownloadURL(
    char* dest, int size, char* format, char* proto, u32 unknown, u32 course,
    u32 timeMillis
)
{
    SakeInterceptGhostDownloadURLBody(
        dest, size, out, proto, unknown, course, timeMillis
    );
}

WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x80677ae0, 0x8067037c, 0x8067714c, 0x80665e88, DEMOTODO),
    SakeInterceptGhostDownloadURL
);

u32 SakeInterceptGhostDownloadURL2(char* dest, int size, char* format)
{
    SakeInterceptGhostDownloadURLBody(dest, size, out);
}

WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x806758a0, 0x8066e13c, 0x80674f0c, 0x80663c00, DEMOTODO),
    SakeInterceptGhostDownloadURL2
);

u32 SakeInterceptGhostUploadURL(
    char* dest, int size, char* format, char* proto, u32 gameId, u32 regionId,
    u32 courseId, u32 score, u32 pid, char* playerInfoA, char* playerInfoB
)
{
    char fixedFormt[256];
    const char* out = FixHostname(format, fixedFormt);

    WWFC_LOG_INFO_FMT("Replace sake GhostUploadURL base to %s", out);

    return snprintf(
        dest, size, out, proto, gameId, regionId, courseId, score, pid,
        playerInfoA, playerInfoB
    );
}

WWFC_DEFINE_PATCH = Patch::Call(
    WWFC_PATCH_LEVEL_SUPPORT,
    RMCXD_PORT(0x80677500, 0x8066fd9c, 0x80676b6c, 0x806658a8, DEMOTODO),
    SakeInterceptGhostUploadURL
);

#endif

} // namespace wwfc::Support
