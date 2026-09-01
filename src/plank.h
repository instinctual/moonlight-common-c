#pragma once

#include <stdint.h>

#define PLANK_RAW_HID_WIRE_MAGIC 0x504c5748U
#define PLANK_RAW_HID_WIRE_VERSION 2U
#define PLANK_RAW_HID_MAX_INTERFACES 16U
#define PLANK_RAW_HID_MAX_DESCRIPTOR_SIZE 4096U
#define PLANK_RAW_HID_MAX_REPORT_SIZE 4096U
#define PLANK_RAW_HID_MAX_PAYLOAD_SIZE (PLANK_RAW_HID_MAX_REPORT_SIZE + 64U)

#define PLANK_CURSOR_WIRE_MAGIC 0x504c4352U
#define PLANK_CURSOR_WIRE_VERSION 1U
#define PLANK_CURSOR_PIXEL_FORMAT_ARGB8888 1U
#define PLANK_CURSOR_MAX_DIMENSION 512U
#define PLANK_CURSOR_MAX_IMAGE_SIZE \
    (PLANK_CURSOR_MAX_DIMENSION * PLANK_CURSOR_MAX_DIMENSION * 4U)
#define PLANK_CURSOR_MAX_CHUNK_SIZE (48U * 1024U)
#define PLANK_CURSOR_CLIENT_FEATURE_FLAG 0x10U
#define PLANK_CURSOR_HOST_FEATURE_FLAG 0x40U

#define PLANK_CURSOR_FLAG_VISIBLE 0x00000001U
#define PLANK_CURSOR_FLAG_FIRST_CHUNK 0x00000002U
#define PLANK_CURSOR_FLAG_LAST_CHUNK 0x00000004U

#define PLANK_CURSOR_POSITION_WIRE_MAGIC 0x504c4350U
#define PLANK_CURSOR_POSITION_WIRE_VERSION 1U

typedef enum _PLANK_RAW_HID_MESSAGE_TYPE {
    PLANK_RAW_HID_DEVICE = 1,
    PLANK_RAW_HID_DESCRIPTOR = 2,
    PLANK_RAW_HID_INPUT = 3,
    PLANK_RAW_HID_GET_REPORT = 4,
    PLANK_RAW_HID_GET_REPORT_REPLY = 5,
    PLANK_RAW_HID_SET_REPORT = 6,
    PLANK_RAW_HID_SET_REPORT_REPLY = 7,
    PLANK_RAW_HID_OUTPUT = 8,
    PLANK_RAW_HID_DETACH = 9,
    PLANK_RAW_HID_ATTACH_RESULT = 10,
    PLANK_RAW_HID_OPEN = 11,
    PLANK_RAW_HID_CLOSE = 12,
    PLANK_RAW_HID_SUSPEND = 13,
} PLANK_RAW_HID_MESSAGE_TYPE;

#pragma pack(push, 1)

typedef struct _PLANK_RAW_HID_WIRE_HEADER {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint16_t interfaceId;
    uint16_t generation;
    uint32_t transactionId;
    uint32_t payloadLength;
} PLANK_RAW_HID_WIRE_HEADER, *PPLANK_RAW_HID_WIRE_HEADER;

typedef struct _PLANK_RAW_HID_DEVICE_MESSAGE {
    uint16_t interfaceCount;
    uint16_t bus;
    uint32_t vendor;
    uint32_t product;
    uint32_t version;
    uint32_t country;
    char name[128];
    char physical[64];
    char unique[64];
} PLANK_RAW_HID_DEVICE_MESSAGE, *PPLANK_RAW_HID_DEVICE_MESSAGE;

typedef struct _PLANK_CURSOR_WIRE_HEADER {
    uint32_t magic;
    uint16_t version;
    uint16_t pixelFormat;
    uint32_t flags;
    uint64_t generation;
    uint32_t width;
    uint32_t height;
    uint32_t hotspotX;
    uint32_t hotspotY;
    uint32_t imageSize;
    uint32_t chunkOffset;
    uint32_t chunkSize;
} PLANK_CURSOR_WIRE_HEADER, *PPLANK_CURSOR_WIRE_HEADER;

typedef struct _PLANK_CURSOR_POSITION_WIRE_MESSAGE {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint64_t sequence;
    uint32_t x;
    uint32_t y;
    uint32_t frameWidth;
    uint32_t frameHeight;
} PLANK_CURSOR_POSITION_WIRE_MESSAGE, *PPLANK_CURSOR_POSITION_WIRE_MESSAGE;

#pragma pack(pop)
