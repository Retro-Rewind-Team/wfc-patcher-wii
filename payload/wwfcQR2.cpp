#include "import/dwc.h"
#include "import/qr2.h"
#include "import/revolution.h"
#include "wwfcLog.hpp"
#include "wwfcPatch.hpp"
#include "wwfcUtil.h"

namespace wwfc
{

#define QR_MAGIC_1 0xFE
#define QR_MAGIC_2 0xFD

#define PACKET_KICK_ORDER 0x11

#if RMC

WWFC_DEFINE_PATCH = {
    Patch::Call( //
        WWFC_PATCH_LEVEL_CRITICAL, //
        RMCXD_PORT(0x800d879c, 0x800d86fc, 0x800d86bc, 0x800d87fc), //
        // clang-format off
        [](QR2::qr2_t qrec,
            char* query,
            int len,
            struct sockaddr* sender) -> void {

            LONGCALL void qr2_parse_queryA(
                QR2::qr2_t qrec, char* query, int len, struct sockaddr* sender
            ) AT(RMCXD_PORT(0x80110720, 0x80110680, 0x80110640, 0x80110798));

            LONGCALL char *SOINetNToA(
                u32 ip
            ) AT(RMCXD_PORT(0x801ed938, 0x801ed898, 0x801ed858, 0x801edd60));

            if (!qrec || !sender || query[0] == ';' || len < 7)
                return qr2_parse_queryA(qrec, query, len, sender);

            if ((u8)query[0] != QR_MAGIC_1 || (u8)query[1] != QR_MAGIC_2)
                return qr2_parse_queryA(qrec, query, len, sender);

            u32 senderAddr = ((RVL::SOSockAddrIn*)sender)->addr.addr;
            if (qrec->hbaddr.addr.addr != senderAddr) {
                WWFC_LOG_INFO("QR2: Packet sent by non-master server.");
                return qr2_parse_queryA(qrec, query, len, sender);
            }

            char command = query[2];
            switch (command) {
                // The server orders us to cut the connection to a PID
                case PACKET_KICK_ORDER: {
                    if (len < 12 || !DWC::stpMatchCnt)
                        break;

                    // Length byte comes after 7 byte qr2 header
                    u8 playerCount = *(u8*)&query[7];

                    int i = 8;
                    u8 j = 0;

                    WWFC_LOG_INFO_FMT("QR2: Received kick order! Contains %d players.", playerCount);

                    // Repeated sequence of u32 pairs starting at byte 9 (index 8)
                    for (; i + 8 < len && j < playerCount; i += 8, j++) {
                        u32 pid = *(u32*)&query[i];
                        u32 ip = *(u32*)&query[i + 4];

                        WWFC_LOG_INFO_FMT("QR2: Received kick order for PID %d, IP %s : %d", pid, SOINetNToA(ip), ip);

                        DWC::DWCiNodeInfo* nodes = DWC::stpMatchCnt->nodes;
                        for (int i = 0; i < 32; i++) {
                            DWC::DWCiNodeInfo node = nodes[i];

                            WWFC_LOG_INFO_FMT("Testing against PID %d, IP %d", node.profileId, node.ipAddr);

                            // If either matches, then we kick. Otherwise skip iter
                            if (node.ipAddr != ip && node.profileId != pid)
                                continue;

                            // If the IP to remove is the host or ourself, DC from the room
                            // Have to check both the node pid and the actual pid, in case of spoofing
                            if (node.profileId == DWC::stpMatchCnt->serverProfileID
                                || pid == DWC::stpMatchCnt->serverProfileID
                                || node.profileId == DWC::stpMatchCnt->profileID
                                || pid == DWC::stpMatchCnt->profileID) {
                                DWC::DWCi_HandleGPError(3);
                                DWC::DWCi_SetError(6, -83337);
                                WWFC_LOG_INFO_FMT("Kick order matched host or yourself: PID %d, IP %d", node.profileId, node.ipAddr);
                                break;
                            }

                            WWFC_LOG_INFO_FMT("Closing connection for PID %d, IP %d", node.profileId, node.ipAddr);
                            DWC::DWC_CloseConnectionHard(nodes[i].aid);
                        }
                    }

                    break;
                }
                default:
                    break;
            }

            return qr2_parse_queryA(qrec, query, len, sender);
        }
        // clang-format on
    ),
};

#endif

} // namespace wwfc
