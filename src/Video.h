#pragma once

#include "LinkedBlockingQueue.h"

typedef struct _QUEUED_DECODE_UNIT {
    DECODE_UNIT decodeUnit;
    LINKED_BLOCKING_QUEUE_ENTRY entry;
} QUEUED_DECODE_UNIT, *PQUEUED_DECODE_UNIT;

#pragma pack(push, 1)

// Fields are big-endian
typedef struct _SS_PING {
    char payload[16];
    uint32_t sequenceNumber;
} SS_PING, *PSS_PING;

#pragma pack(pop)
