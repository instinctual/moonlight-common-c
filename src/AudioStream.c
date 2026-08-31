#include "Limelight-internal.h"

static RTP_AUDIO_STATS nativeAudioStats;

int initializeAudioStream(void) {
    memset(&nativeAudioStats, 0, sizeof(nativeAudioStats));
    return 0;
}

void destroyAudioStream(void) {
}

int LiSubmitStationConnectAudioPacket(const unsigned char* packet,
                                      int packetLength,
                                      uint16_t frameSamples,
                                      uint32_t missingSamples) {
    if (frameSamples == 0 || packetLength < 0 ||
            (packetLength != 0 && packet == NULL) ||
            (missingSamples != 0 && packetLength != 0)) {
        return -1;
    }

    if (missingSamples != 0) {
        uint32_t missingFrames =
            (missingSamples + frameSamples - 1) / frameSamples;
        while (missingFrames-- != 0) {
            AudioCallbacks.decodeAndPlaySample(NULL, 0);
        }
    }
    else {
        AudioCallbacks.decodeAndPlaySample((char*)packet, packetLength);
    }

    return 0;
}

void stopAudioStream(void) {
    AudioCallbacks.stop();
    AudioCallbacks.cleanup();
}

int startAudioStream(void* audioContext, int arFlags) {
    OPUS_MULTISTREAM_CONFIGURATION chosenConfig;

    if (HighQualitySurroundEnabled) {
        LC_ASSERT(HighQualitySurroundSupported);
        LC_ASSERT(HighQualityOpusConfig.channelCount != 0);
        LC_ASSERT(HighQualityOpusConfig.streams != 0);
        chosenConfig = HighQualityOpusConfig;
    }
    else {
        LC_ASSERT(NormalQualityOpusConfig.channelCount != 0);
        LC_ASSERT(NormalQualityOpusConfig.streams != 0);
        chosenConfig = NormalQualityOpusConfig;
    }

    chosenConfig.samplesPerFrame = 48 * AudioPacketDuration;
    const int err = AudioCallbacks.init(StreamConfig.audioConfiguration,
                                        &chosenConfig, audioContext, arFlags);
    if (err != 0) {
        return err;
    }

    AudioCallbacks.start();
    return 0;
}

int LiGetPendingAudioFrames(void) {
    return 0;
}

int LiGetPendingAudioDuration(void) {
    return 0;
}

const RTP_AUDIO_STATS* LiGetRTPAudioStats(void) {
    return &nativeAudioStats;
}
