#pragma once

#include "Video.h"

// Temporary packet-entry shape retained only by the dormant RTP depacketizer
// adapter. Native StationConnect video submits complete reconstructed frames
// through LiSubmitStationConnectVideoFrame().
typedef struct _RTPV_QUEUE_ENTRY {
    struct _RTPV_QUEUE_ENTRY* next;
    struct _RTPV_QUEUE_ENTRY* prev;
    PRTP_PACKET packet;
    uint64_t receiveTimeUs;
    uint64_t presentationTimeUs;
    uint32_t rtpTimestamp;
    int length;
    bool isParity;
} RTPV_QUEUE_ENTRY, *PRTPV_QUEUE_ENTRY;
