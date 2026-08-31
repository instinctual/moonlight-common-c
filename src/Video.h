#pragma once

#include "LinkedBlockingQueue.h"

typedef struct _QUEUED_DECODE_UNIT {
    DECODE_UNIT decodeUnit;
    LINKED_BLOCKING_QUEUE_ENTRY entry;
} QUEUED_DECODE_UNIT, *PQUEUED_DECODE_UNIT;

#pragma pack(push, 1)

// The encrypted video header must be a multiple
// of 16 bytes in size to ensure the block size
// for FEC stays a multiple of 16 too.
typedef struct _ENC_VIDEO_HEADER {
    uint8_t iv[12];
    uint32_t frameNumber;
    uint8_t tag[16];
} ENC_VIDEO_HEADER, *PENC_VIDEO_HEADER;

// Fields are big-endian
typedef struct _SS_PING {
    char payload[16];
    uint32_t sequenceNumber;
} SS_PING, *PSS_PING;

#pragma pack(pop)
