#pragma once

#include "gamespy.h"
#include "import/revolution.h"

#ifdef __cplusplus
extern "C" {

namespace QR2
{
#endif

typedef struct qr2_implementation_s* qr2_t;

#define REQUEST_KEY_LEN 4
#define RECENT_CLIENT_MESSAGES_TO_TRACK 10
#define QR2_IPVERIFY_ARRAY_SIZE                                                \
    200 // allowed outstanding queryies in those 4 seconds

// https://github.com/em-eight/mkw/blob/b2fa6eb775b57c4f81bb7a3312169810c3c032ef/source/gamespy/qr2/qr2.h#L375
struct qr2_ipverify_info_s {
    RVL::SOSockAddrIn addr; // addr = 0 when not in use
    u32 challenge;
    u32 createtime;
};

// https://github.com/em-eight/mkw/blob/master/source/gamespy/qr2/qr2.h#L86
struct qr2_implementation_s {
    int hbsock; // 0x00..0x04
    char gamename[64]; // 0x04..0x44
    char secret_key[64]; // 0x44..0x84
    char instance_key[REQUEST_KEY_LEN]; // 0x84..0x8c
    void* server_key_callback; // 0x8c..0x90
    void* player_key_callback;
    void* team_key_callback;
    void* key_list_callback;
    void* playerteam_count_callback;
    void* adderror_callback;
    void* nn_callback; // 0xa0
    void* cm_callback; // 0xa4
    void* pa_callback; // 0xa8
    u32 lastheartbeat; // 0xac
    u32 lastka; // 0xb0
    int userstatechangerequested; // 0xb4
    int listed_state; // 0xb8
    int ispublic; // 0xbc
    int qport; // 0xc0
    int read_socket; // 0xc4
    int nat_negotiate; // 0xc8
    RVL::SOSockAddrIn hbaddr;
    void* cdkeyprocess;
    int client_message_keys[RECENT_CLIENT_MESSAGES_TO_TRACK];
    int cur_message_key;
    unsigned int publicip;
    unsigned short publicport;
    void* udata;

    u8 backendoptions;
    struct qr2_ipverify_info_s ipverify[QR2_IPVERIFY_ARRAY_SIZE];
};

static_assert(sizeof(qr2_implementation_s) == 0xd94);

#ifdef __cplusplus
}
}
#endif
