#pragma once

#include <stdint.h>

#define SC_RAW_HID_WIRE_MAGIC 0x53435748U
#define SC_RAW_HID_WIRE_VERSION 2U
#define SC_RAW_HID_MAX_INTERFACES 16U
#define SC_RAW_HID_MAX_DESCRIPTOR_SIZE 4096U
#define SC_RAW_HID_MAX_REPORT_SIZE 4096U
#define SC_RAW_HID_MAX_PAYLOAD_SIZE (SC_RAW_HID_MAX_REPORT_SIZE + 64U)

#define SC_CURSOR_WIRE_MAGIC 0x53434352U
#define SC_CURSOR_WIRE_VERSION 1U
#define SC_CURSOR_PIXEL_FORMAT_ARGB8888 1U
#define SC_CURSOR_MAX_DIMENSION 512U
#define SC_CURSOR_MAX_IMAGE_SIZE \
    (SC_CURSOR_MAX_DIMENSION * SC_CURSOR_MAX_DIMENSION * 4U)
#define SC_CURSOR_MAX_CHUNK_SIZE (48U * 1024U)
#define SC_CURSOR_CLIENT_FEATURE_FLAG 0x10U
#define SC_CURSOR_HOST_FEATURE_FLAG 0x40U

#define SC_CURSOR_FLAG_VISIBLE 0x00000001U
#define SC_CURSOR_FLAG_FIRST_CHUNK 0x00000002U
#define SC_CURSOR_FLAG_LAST_CHUNK 0x00000004U

typedef enum _SC_RAW_HID_MESSAGE_TYPE {
    SC_RAW_HID_DEVICE = 1,
    SC_RAW_HID_DESCRIPTOR = 2,
    SC_RAW_HID_INPUT = 3,
    SC_RAW_HID_GET_REPORT = 4,
    SC_RAW_HID_GET_REPORT_REPLY = 5,
    SC_RAW_HID_SET_REPORT = 6,
    SC_RAW_HID_SET_REPORT_REPLY = 7,
    SC_RAW_HID_OUTPUT = 8,
    SC_RAW_HID_DETACH = 9,
    SC_RAW_HID_ATTACH_RESULT = 10,
    SC_RAW_HID_OPEN = 11,
    SC_RAW_HID_CLOSE = 12,
    SC_RAW_HID_SUSPEND = 13,
} SC_RAW_HID_MESSAGE_TYPE;

#pragma pack(push, 1)

typedef struct _SC_RAW_HID_WIRE_HEADER {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint16_t interfaceId;
    uint16_t generation;
    uint32_t transactionId;
    uint32_t payloadLength;
} SC_RAW_HID_WIRE_HEADER, *PSC_RAW_HID_WIRE_HEADER;

typedef struct _SC_RAW_HID_DEVICE_MESSAGE {
    uint16_t interfaceCount;
    uint16_t bus;
    uint32_t vendor;
    uint32_t product;
    uint32_t version;
    uint32_t country;
    char name[128];
    char physical[64];
    char unique[64];
} SC_RAW_HID_DEVICE_MESSAGE, *PSC_RAW_HID_DEVICE_MESSAGE;

typedef struct _SC_CURSOR_WIRE_HEADER {
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
} SC_CURSOR_WIRE_HEADER, *PSC_CURSOR_WIRE_HEADER;

#pragma pack(pop)
