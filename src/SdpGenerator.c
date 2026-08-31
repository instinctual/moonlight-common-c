#include "Limelight-internal.h"
#include <inttypes.h>

#define MAX_OPTION_NAME_LEN 128

#define MAX_SDP_HEADER_LEN 128
#define MAX_SDP_TAIL_LEN 128

typedef struct _SDP_OPTION {
    char name[MAX_OPTION_NAME_LEN + 1];
    void* payload;
    int payloadLen;
    struct _SDP_OPTION* next;
} SDP_OPTION, *PSDP_OPTION;

// Cleanup the attribute list
static void freeAttributeList(PSDP_OPTION head) {
    PSDP_OPTION next;
    while (head != NULL) {
        next = head->next;
        free(head);
        head = next;
    }
}

// Get the size of the attribute list
static int getSerializedAttributeListSize(PSDP_OPTION head) {
    PSDP_OPTION currentEntry = head;
    size_t size = 0;
    while (currentEntry != NULL) {
        size += strlen("a=");
        size += strlen(currentEntry->name);
        size += strlen(":");
        size += currentEntry->payloadLen;
        size += strlen(" \r\n");

        currentEntry = currentEntry->next;
    }
    // Add one for the null terminator
    return (int)size + 1;
}

// Populate the serialized attribute list into a string
static int fillSerializedAttributeList(char* buffer, size_t length, PSDP_OPTION head) {
    PSDP_OPTION currentEntry = head;
    int offset = 0;
    while (currentEntry != NULL) {
        int ret = snprintf(&buffer[offset], length, "a=%s:", currentEntry->name);
        if (ret > 0 && (size_t)ret < length) {
            offset += ret;
            length -= ret;
        }
        else {
            LC_ASSERT(false);
            break;
        }

        if ((size_t)currentEntry->payloadLen < length) {
            memcpy(&buffer[offset], currentEntry->payload, currentEntry->payloadLen);
            offset += currentEntry->payloadLen;
            length -= currentEntry->payloadLen;
        }
        else {
            LC_ASSERT(false);
            break;
        }

        ret = snprintf(&buffer[offset], length, " \r\n");
        if (ret > 0 && (size_t)ret < length) {
            offset += ret;
            length -= ret;
        }
        else {
            LC_ASSERT(false);
            break;
        }

        currentEntry = currentEntry->next;
    }

    // We should have only space for the null terminator left over
    LC_ASSERT(length == 1);
    return offset;
}

// Add an attribute
static int addAttributeBinary(PSDP_OPTION* head, char* name, const void* payload, int payloadLen) {
    PSDP_OPTION option, currentOption;

    option = malloc(sizeof(*option) + payloadLen);
    if (option == NULL) {
        return -1;
    }

    if (!PltSafeStrcpy(option->name, sizeof(option->name), name)) {
        free(option);
        return -1;
    }

    option->next = NULL;
    option->payloadLen = payloadLen;
    option->payload = (void*)(option + 1);
    memcpy(option->payload, payload, payloadLen);

    if (*head == NULL) {
        *head = option;
    }
    else {
        currentOption = *head;
        while (currentOption->next != NULL) {
            currentOption = currentOption->next;
        }
        currentOption->next = option;
    }

    return 0;
}

// Add an attribute string
static int addAttributeString(PSDP_OPTION* head, char* name, const char* payload) {
    // We purposefully omit the null terminating character
    return addAttributeBinary(head, name, payload, (int)strlen(payload));
}

static int addGen3Options(PSDP_OPTION* head, char* addrStr) {
    int payloadInt;
    int err = 0;

    err |= addAttributeString(head, "x-nv-general.serverAddress", addrStr);

    payloadInt = htonl(0x42774141);
    err |= addAttributeBinary(head,
        "x-nv-general.featureFlags", &payloadInt, sizeof(payloadInt));

    payloadInt = htonl(0x41514141);
    err |= addAttributeBinary(head,
        "x-nv-video[0].transferProtocol", &payloadInt, sizeof(payloadInt));
    err |= addAttributeBinary(head,
        "x-nv-video[1].transferProtocol", &payloadInt, sizeof(payloadInt));
    err |= addAttributeBinary(head,
        "x-nv-video[2].transferProtocol", &payloadInt, sizeof(payloadInt));
    err |= addAttributeBinary(head,
        "x-nv-video[3].transferProtocol", &payloadInt, sizeof(payloadInt));

    payloadInt = htonl(0x42414141);
    err |= addAttributeBinary(head,
        "x-nv-video[0].rateControlMode", &payloadInt, sizeof(payloadInt));
    payloadInt = htonl(0x42514141);
    err |= addAttributeBinary(head,
        "x-nv-video[1].rateControlMode", &payloadInt, sizeof(payloadInt));
    err |= addAttributeBinary(head,
        "x-nv-video[2].rateControlMode", &payloadInt, sizeof(payloadInt));
    err |= addAttributeBinary(head,
        "x-nv-video[3].rateControlMode", &payloadInt, sizeof(payloadInt));

    err |= addAttributeString(head, "x-nv-vqos[0].bw.flags", "14083");

    err |= addAttributeString(head, "x-nv-vqos[0].videoQosMaxConsecutiveDrops", "0");
    err |= addAttributeString(head, "x-nv-vqos[1].videoQosMaxConsecutiveDrops", "0");
    err |= addAttributeString(head, "x-nv-vqos[2].videoQosMaxConsecutiveDrops", "0");
    err |= addAttributeString(head, "x-nv-vqos[3].videoQosMaxConsecutiveDrops", "0");

    return err;
}

static int addGen4Options(PSDP_OPTION* head, char* addrStr) {
    char payloadStr[92];
    int err = 0;

    LC_ASSERT(RtspPortNumber != 0);
    snprintf(payloadStr, sizeof(payloadStr), "rtsp://%s:%u", addrStr, RtspPortNumber);
    err |= addAttributeString(head, "x-nv-general.serverAddress", payloadStr);

    return err;
}

static int addGen5Options(PSDP_OPTION* head) {
    int err = 0;
    
    if (APP_VERSION_AT_LEAST(7, 1, 446) && (StreamConfig.width < 720 || StreamConfig.height < 540)) {
        // We enable DRC with a static DRC table for very low resoutions on GFE 3.26 to work around
        // a bug that causes nvstreamer.exe to crash due to failing to populate a list of valid resolutions.
        //
        // Despite the fact that the DRC table doesn't include our target streaming resolution, we still
        // seem to stream at the target resolution, presumably because we don't send control data to tell
        // the host otherwise.
        err |= addAttributeString(head, "x-nv-vqos[0].drc.enable", "1");
        err |= addAttributeString(head, "x-nv-vqos[0].drc.tableType", "2");
    }
    else {
        // Disable dynamic resolution switching
        err |= addAttributeString(head, "x-nv-vqos[0].drc.enable", "0");
    }

    // Recovery mode can cause the FEC percentage to change mid-frame, which
    // breaks many assumptions in RTP FEC queue.
    err |= addAttributeString(head, "x-nv-general.enableRecoveryMode", "0");

    return err;
}

static PSDP_OPTION getAttributesList(char*urlSafeAddr) {
    PSDP_OPTION optionHead;
    char payloadStr[92];
    int audioChannelCount;
    int audioChannelMask;
    int err;
    int adjustedBitrate;

    // This must have been resolved to either local or remote by now
    LC_ASSERT(StreamConfig.streamingRemotely != STREAM_CFG_AUTO);

    optionHead = NULL;
    err = 0;

    if (IS_SUNSHINE()) {
        // Send client feature flags to Sunshine hosts
        uint32_t moonlightFeatureFlags = ML_FF_LOCAL_CURSOR;
        if (StreamConfig.colorSpace == COLORSPACE_IDENTITY_GBR &&
                StreamConfig.colorRange == COLOR_RANGE_FULL &&
                (NegotiatedVideoFormat & VIDEO_FORMAT_MASK_YUV444)) {
            moonlightFeatureFlags |= ML_FF_IDENTITY_GBR_444;
        }
        snprintf(payloadStr, sizeof(payloadStr), "%" PRIu32, moonlightFeatureFlags);
        err |= addAttributeString(&optionHead, "x-ml-general.featureFlags", payloadStr);

        // Enable YUV444 if requested
        if (NegotiatedVideoFormat & VIDEO_FORMAT_MASK_YUV444) {
            err |= addAttributeString(&optionHead, "x-ss-video[0].chromaSamplingType", "1");
        }
        else if (NegotiatedVideoFormat & VIDEO_FORMAT_MASK_YUV422) {
            err |= addAttributeString(&optionHead, "x-ss-video[0].chromaSamplingType", "2");
        }
        else {
            err |= addAttributeString(&optionHead, "x-ss-video[0].chromaSamplingType", "0");
        }
    }

    snprintf(payloadStr, sizeof(payloadStr), "%d", StreamConfig.width);
    err |= addAttributeString(&optionHead, "x-nv-video[0].clientViewportWd", payloadStr);
    snprintf(payloadStr, sizeof(payloadStr), "%d", StreamConfig.height);
    err |= addAttributeString(&optionHead, "x-nv-video[0].clientViewportHt", payloadStr);

    snprintf(payloadStr, sizeof(payloadStr), "%d", StreamConfig.fps);
    err |= addAttributeString(&optionHead, "x-nv-video[0].maxFPS", payloadStr);

    snprintf(payloadStr, sizeof(payloadStr), "%d", StreamConfig.packetSize);
    err |= addAttributeString(&optionHead, "x-nv-video[0].packetSize", payloadStr);

    err |= addAttributeString(&optionHead, "x-nv-video[0].rateControlMode", "4");

    err |= addAttributeString(&optionHead, "x-nv-video[0].timeoutLengthMs", "7000");
    err |= addAttributeString(&optionHead, "x-nv-video[0].framesWithInvalidRefThreshold", "0");

    // 20% of the video bitrate will added to the user-specified bitrate for FEC
    adjustedBitrate = (int)(StreamConfig.bitrate * 0.80);

    // Use more strict bitrate logic when streaming remotely. The theory here is that remote
    // streaming is much more bandwidth sensitive. Someone might select 5 Mbps because that's
    // really all they have, so we need to be careful not to exceed the cap, even counting
    // things like audio and control data.
    if (StreamConfig.streamingRemotely == STREAM_CFG_REMOTE) {
        // Subtract 500 Kbps to leave room for audio and control. On remote streams,
        // GFE will use 96Kbps stereo audio. For local streams, it will choose 512Kbps.
        if (adjustedBitrate > 500) {
            adjustedBitrate -= 500;
        }
    }

    // GFE currently imposes a limit of 100 Mbps for the video bitrate. It will automatically
    // impose that on maximumBitrateKbps but not on initialBitrateKbps. We will impose the cap
    // ourselves so initialBitrateKbps does not exceed maximumBitrateKbps.
    adjustedBitrate = adjustedBitrate > 100000 ? 100000 : adjustedBitrate;

    // We don't support dynamic bitrate scaling properly (it tends to bounce between min and max and never
    // settle on the optimal bitrate if it's somewhere in the middle), so we'll just latch the bitrate
    // to the requested value.
    if (AppVersionQuad[0] >= 5) {
        snprintf(payloadStr, sizeof(payloadStr), "%d", adjustedBitrate);

        err |= addAttributeString(&optionHead, "x-nv-video[0].initialBitrateKbps", payloadStr);
        err |= addAttributeString(&optionHead, "x-nv-video[0].initialPeakBitrateKbps", payloadStr);

        err |= addAttributeString(&optionHead, "x-nv-vqos[0].bw.minimumBitrateKbps", payloadStr);
        err |= addAttributeString(&optionHead, "x-nv-vqos[0].bw.maximumBitrateKbps", payloadStr);

        // Send the configured bitrate to Sunshine hosts, so they can adjust for dynamic FEC percentage
        if (IS_SUNSHINE()) {
            snprintf(payloadStr, sizeof(payloadStr), "%u", StreamConfig.bitrate);
            err |= addAttributeString(&optionHead, "x-ml-video.configuredBitrateKbps", payloadStr);

            // StationConnect treats this as the exact encoder target rather
            // than a total network budget. This makes the first encoder agree
            // with the toolbar before the live control stream is available.
            if (SunshineFeatureFlags & LI_FF_ENCODER_TARGET_ACK) {
                err |= addAttributeString(&optionHead, "x-sc-video.encoderTargetKbps", payloadStr);
            }
        }
    }
    else {
        if (StreamConfig.streamingRemotely == STREAM_CFG_REMOTE) {
            err |= addAttributeString(&optionHead, "x-nv-video[0].averageBitrate", "4");
            err |= addAttributeString(&optionHead, "x-nv-video[0].peakBitrate", "4");
        }

        snprintf(payloadStr, sizeof(payloadStr), "%d", adjustedBitrate);
        err |= addAttributeString(&optionHead, "x-nv-vqos[0].bw.minimumBitrate", payloadStr);
        err |= addAttributeString(&optionHead, "x-nv-vqos[0].bw.maximumBitrate", payloadStr);
    }
    
    // FEC must be enabled for proper packet sequencing to be done by RTP FEC queue
    err |= addAttributeString(&optionHead, "x-nv-vqos[0].fec.enable", "1");
    
    err |= addAttributeString(&optionHead, "x-nv-vqos[0].videoQualityScoreUpdateTime", "5000");

    // If the remote host is local (RFC 1918), enable QoS tagging for our traffic. Windows qWave
    // will disable it if the host is off-link, *however* Windows may get it wrong in cases where
    // the host is directly connected to the Internet without a NAT. In this case, it may send DSCP
    // marked traffic off-link and it could lead to black holes due to misconfigured ISP hardware
    // or CPE. For this reason, we only enable it in cases where it looks like it will work.
    //
    // Even though IPv6 hardware should be much less likely to have this issue, we can't tell
    // if our address is a NAT64 synthesized IPv6 address or true end-to-end IPv6. If it's the
    // former, it may have the same problem as other IPv4 traffic.
    if (StreamConfig.streamingRemotely == STREAM_CFG_LOCAL) {
        err |= addAttributeString(&optionHead, "x-nv-vqos[0].qosTrafficType", "5");
        err |= addAttributeString(&optionHead, "x-nv-aqos.qosTrafficType", "4");
    }
    else {
        err |= addAttributeString(&optionHead, "x-nv-vqos[0].qosTrafficType", "0");
        err |= addAttributeString(&optionHead, "x-nv-aqos.qosTrafficType", "0");
    }

    if (AppVersionQuad[0] == 3) {
        err |= addGen3Options(&optionHead, urlSafeAddr);
    }
    else if (AppVersionQuad[0] == 4) {
        err |= addGen4Options(&optionHead, urlSafeAddr);
    }
    else {
        err |= addGen5Options(&optionHead);
    }

    audioChannelCount = CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(StreamConfig.audioConfiguration);
    audioChannelMask = CHANNEL_MASK_FROM_AUDIO_CONFIGURATION(StreamConfig.audioConfiguration);

    if (AppVersionQuad[0] >= 4) {
        unsigned char slicesPerFrame;

        // Use slicing for increased performance on some decoders
        slicesPerFrame = (unsigned char)(VideoCallbacks.capabilities >> 24);
        if (slicesPerFrame == 0) {
            // If not using slicing, we request 1 slice per frame
            slicesPerFrame = 1;
        }
        snprintf(payloadStr, sizeof(payloadStr), "%d", slicesPerFrame);
        err |= addAttributeString(&optionHead, "x-nv-video[0].videoEncoderSlicesPerFrame", payloadStr);

        if (NegotiatedVideoFormat & VIDEO_FORMAT_MASK_AV1) {
            err |= addAttributeString(&optionHead, "x-nv-vqos[0].bitStreamFormat", "2");
        }
        else if (NegotiatedVideoFormat & VIDEO_FORMAT_MASK_H265) {
            err |= addAttributeString(&optionHead, "x-nv-clientSupportHevc", "1");
            err |= addAttributeString(&optionHead, "x-nv-vqos[0].bitStreamFormat", "1");

            if (!APP_VERSION_AT_LEAST(7, 1, 408)) {
                // This disables split frame encode on GFE 3.10 which seems to produce broken
                // HEVC output at 1080p60 (full of artifacts even on the SHIELD itself, go figure).
                // It now appears to work fine on GFE 3.14.1.
                Limelog("Disabling split encode for HEVC on older GFE version");
                err |= addAttributeString(&optionHead, "x-nv-video[0].encoderFeatureSetting", "0");
            }
        }
        else {
            err |= addAttributeString(&optionHead, "x-nv-clientSupportHevc", "0");
            err |= addAttributeString(&optionHead, "x-nv-vqos[0].bitStreamFormat", "0");
        }

        if (AppVersionQuad[0] >= 7) {
            // Enable HDR if requested
            if (NegotiatedVideoFormat & VIDEO_FORMAT_MASK_10BIT) {
                err |= addAttributeString(&optionHead, "x-nv-video[0].dynamicRangeMode", "1");
            }
            else {
                err |= addAttributeString(&optionHead, "x-nv-video[0].dynamicRangeMode", "0");
            }

            // If the decoder supports reference frame invalidation, that indicates it also supports
            // the maximum number of reference frames allowed by the codec. Even if we can't use RFI
            // due to lack of host support, we can still allow the host to pick a number of reference
            // frames greater than 1 to improve encoding efficiency.
            if (isReferenceFrameInvalidationSupportedByDecoder()) {
                err |= addAttributeString(&optionHead, "x-nv-video[0].maxNumReferenceFrames", "0");
            }
            else {
                // Restrict the video stream to 1 reference frame if we're not using
                // reference frame invalidation. This helps to improve compatibility with
                // some decoders that don't like the default of having 16 reference frames.
                err |= addAttributeString(&optionHead, "x-nv-video[0].maxNumReferenceFrames", "1");
            }

            snprintf(payloadStr, sizeof(payloadStr), "%d", StreamConfig.clientRefreshRateX100);
            err |= addAttributeString(&optionHead, "x-nv-video[0].clientRefreshRateX100", payloadStr);
        }

        snprintf(payloadStr, sizeof(payloadStr), "%d", audioChannelCount);
        err |= addAttributeString(&optionHead, "x-nv-audio.surround.numChannels", payloadStr);
        snprintf(payloadStr, sizeof(payloadStr), "%d", audioChannelMask);
        err |= addAttributeString(&optionHead, "x-nv-audio.surround.channelMask", payloadStr);
        if (audioChannelCount > 2) {
            err |= addAttributeString(&optionHead, "x-nv-audio.surround.enable", "1");
        }
        else {
            err |= addAttributeString(&optionHead, "x-nv-audio.surround.enable", "0");
        }
    }

    if (AppVersionQuad[0] >= 7) {
        if (StreamConfig.bitrate >= HIGH_AUDIO_BITRATE_THRESHOLD && audioChannelCount > 2 &&
                HighQualitySurroundSupported && (AudioCallbacks.capabilities & CAPABILITY_SLOW_OPUS_DECODER) == 0) {
            // Enable high quality mode for surround sound
            err |= addAttributeString(&optionHead, "x-nv-audio.surround.AudioQuality", "1");

            // Let the audio stream code know that it needs to disable coupled streams when
            // decoding this audio stream.
            HighQualitySurroundEnabled = true;

            // Use 5 ms frames since we don't have a slow decoder
            AudioPacketDuration = 5;
        }
        else {
            err |= addAttributeString(&optionHead, "x-nv-audio.surround.AudioQuality", "0");
            HighQualitySurroundEnabled = false;

            if ((AudioCallbacks.capabilities & CAPABILITY_SLOW_OPUS_DECODER) ||
                     ((AudioCallbacks.capabilities & CAPABILITY_SUPPORTS_ARBITRARY_AUDIO_DURATION) != 0 &&
                       StreamConfig.bitrate < LOW_AUDIO_BITRATE_TRESHOLD)) {
                // Use 10 ms packets for slow devices and networks to balance latency and bandwidth usage
                AudioPacketDuration = 10;
            }
            else {
                // Use 5 ms packets by default for lowest latency
                AudioPacketDuration = 5;
            }
        }

        snprintf(payloadStr, sizeof(payloadStr), "%d", AudioPacketDuration);
        err |= addAttributeString(&optionHead, "x-nv-aqos.packetDuration", payloadStr);
    }
    else {
        // 5 ms duration for legacy servers
        AudioPacketDuration = 5;

        // High quality audio mode not supported on legacy servers
        HighQualitySurroundEnabled = false;
    }

    if (AppVersionQuad[0] >= 7) {
        snprintf(payloadStr, sizeof(payloadStr), "%d", (StreamConfig.colorSpace << 1) | StreamConfig.colorRange);
        err |= addAttributeString(&optionHead, "x-nv-video[0].encoderCscMode", payloadStr);
    }

    if (err == 0) {
        return optionHead;
    }

    freeAttributeList(optionHead);
    return NULL;
}

// Populate the SDP header with required information
static int fillSdpHeader(char* buffer, size_t length, int rtspClientVersion, char*urlSafeAddr) {
    return snprintf(buffer, length,
        "v=0\r\n"
        "o=android 0 %d IN %s %s\r\n"
        "s=NVIDIA Streaming Client\r\n",
        rtspClientVersion,
        RemoteAddr.ss_family == AF_INET ? "IPv4" : "IPv6",
        urlSafeAddr);
}

// Populate the SDP tail with required information
static int fillSdpTail(char* buffer, size_t length) {
    LC_ASSERT(VideoPortNumber != 0);
    return snprintf(buffer, length,
        "t=0 0\r\n"
        "m=video %d  \r\n",
        AppVersionQuad[0] < 4 ? 47996 : VideoPortNumber);
}

// Get the SDP attributes for the stream config
char* getSdpPayloadForStreamConfig(int rtspClientVersion, int* length) {
    PSDP_OPTION attributeList;
    int attributeListSize;
    int offset, written;
    char* payload;
    char urlSafeAddr[URLSAFESTRING_LEN];

    addrToUrlSafeString(&RemoteAddr, urlSafeAddr, sizeof(urlSafeAddr));

    attributeList = getAttributesList(urlSafeAddr);
    if (attributeList == NULL) {
        return NULL;
    }

    attributeListSize = getSerializedAttributeListSize(attributeList);
    payload = malloc(MAX_SDP_HEADER_LEN + MAX_SDP_TAIL_LEN + attributeListSize);
    if (payload == NULL) {
        freeAttributeList(attributeList);
        return NULL;
    }

    offset = 0;
    written = fillSdpHeader(payload, MAX_SDP_HEADER_LEN, rtspClientVersion, urlSafeAddr);
    if (written < 0 || written >= MAX_SDP_HEADER_LEN) {
        LC_ASSERT(false);
        free(payload);
        freeAttributeList(attributeList);
        return NULL;
    }
    else {
        offset += written;
    }
    written = fillSerializedAttributeList(&payload[offset], attributeListSize, attributeList);
    if (written < 0 || written >= attributeListSize) {
        LC_ASSERT(false);
        free(payload);
        freeAttributeList(attributeList);
        return NULL;
    }
    else {
        offset += written;
    }
    written = fillSdpTail(&payload[offset], MAX_SDP_TAIL_LEN);
    if (written < 0 || written >= MAX_SDP_TAIL_LEN) {
        LC_ASSERT(false);
        free(payload);
        freeAttributeList(attributeList);
        return NULL;
    }
    else {
        offset += written;
    }

    freeAttributeList(attributeList);
    *length = offset;
    return payload;
}
