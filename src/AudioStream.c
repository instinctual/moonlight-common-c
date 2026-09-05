#include "Limelight-internal.h"

static RTP_AUDIO_STATS nativeAudioStats;

int initializeAudioStream(void) {
    memset(&nativeAudioStats, 0, sizeof(nativeAudioStats));
    return 0;
}

void destroyAudioStream(void) {
}

int LiSubmitPlankAudioPacket(const unsigned char* packet,
                                      int packetLength,
                                      uint16_t frameSamples,
                                      uint32_t missingSamples,
                                      uint64_t sourcePtsUs) {
    if (frameSamples == 0 || packetLength < 0 ||
            (packetLength != 0 && packet == NULL) ||
            (missingSamples != 0 && packetLength != 0) ||
            (missingSamples == 0 && sourcePtsUs >= (UINT64_C(1) << 61))) {
        return -1;
    }

    if (missingSamples != 0) {
        uint32_t missingFrames =
            missingSamples / frameSamples + (missingSamples % frameSamples != 0);
        while (missingFrames-- != 0) {
            AudioCallbacks.decodeAndPlaySample(NULL, 0, PLANK_AUDIO_PTS_UNKNOWN);
        }
    }
    else {
        AudioCallbacks.decodeAndPlaySample((char*)packet, packetLength, sourcePtsUs);
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
