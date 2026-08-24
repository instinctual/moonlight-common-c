#pragma once

#include <stdint.h>

#define SC_RAW_HID_WIRE_MAGIC 0x53435748U
#define SC_RAW_HID_WIRE_VERSION 2U
#define SC_RAW_HID_MAX_INTERFACES 16U
#define SC_RAW_HID_MAX_DESCRIPTOR_SIZE 4096U
#define SC_RAW_HID_MAX_REPORT_SIZE 4096U
#define SC_RAW_HID_MAX_PAYLOAD_SIZE (SC_RAW_HID_MAX_REPORT_SIZE + 64U)

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

#pragma pack(pop)
