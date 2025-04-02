#include "import/dwc.h"
#include "import/mkw/net/net.hpp"
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
        [](void* qrec,
           char* query,
           int len,
           struct sockaddr* sender) -> void {

            LONGCALL void qr2_parse_queryA(
                void* qrec, char* query, int len, struct sockaddr* sender
            ) AT(RMCXD_PORT(0x80110720, 0x80110680, 0x80110640, 0x80110798));

            if (!qrec || query[0] == ';')
                return qr2_parse_queryA(qrec, query, len, sender);

            if (len < 7)
                return;

            if ((u8)query[0] != QR_MAGIC_1 || (u8)query[1] != QR_MAGIC_2)
                return qr2_parse_queryA(qrec, query, len, sender);

            char command = query[2];
            switch (command) {
                // The server orders us to cut the connection to a PID
                case PACKET_KICK_ORDER:
                    if (len >= 12 && DWC::stpMatchCnt) {
                        u32 pid = *(u32*)&query[8];

                        // If the PID to remove is the host or ourself, DC from the room
                        if (pid == DWC::stpMatchCnt->serverProfileID || pid == DWC::stpMatchCnt->profileID) {
                            DWC::DWCi_HandleGPError(3);
                            DWC::DWCi_SetError(6, -83337);
                            break;
                        } 
                        // If the PID is another peer, close the connection to them
                        else {                 
                            DWC::DWCiNodeInfo* nodes = DWC::stpMatchCnt->nodes;
                            for (int i = 0; i < 32; i++) {
                                if (nodes[i].profileId == pid) {
                                    DWC::DWC_CloseConnectionHard(nodes[i].aid);
                                    break;
                                }
                            }
                        }
                    }
                    break;
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