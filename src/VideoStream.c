#include "Limelight-internal.h"

static PLT_THREAD decoderThread;
static RTP_VIDEO_STATS nativeVideoStats;

static void VideoDecoderThreadProc(void* context) {
    while (!PltIsThreadInterrupted(&decoderThread)) {
        VIDEO_FRAME_HANDLE frameHandle;
        PDECODE_UNIT decodeUnit;

        if (!LiWaitForNextVideoFrame(&frameHandle, &decodeUnit)) {
            return;
        }

        LiCompleteVideoFrame(frameHandle,
                             VideoCallbacks.submitDecodeUnit(decodeUnit));
    }
}

void initializeVideoStream(void) {
    initializeVideoDepacketizer(StreamConfig.packetSize);
    memset(&nativeVideoStats, 0, sizeof(nativeVideoStats));
}

void destroyVideoStream(void) {
    destroyVideoDepacketizer();
}

void notifyKeyFrameReceived(void) {
}

int readFirstFrame(void) {
    return 0;
}

void stopVideoStream(void) {
    VideoCallbacks.stop();
    stopVideoDepacketizer();

    if ((VideoCallbacks.capabilities &
            (CAPABILITY_DIRECT_SUBMIT | CAPABILITY_PULL_RENDERER)) == 0) {
        PltInterruptThread(&decoderThread);
        PltJoinThread(&decoderThread);
    }

    VideoCallbacks.cleanup();
}

int startVideoStream(void* rendererContext, int drFlags) {
    int err;

    LC_ASSERT(NegotiatedVideoFormat != 0);
    err = VideoCallbacks.setup(NegotiatedVideoFormat, StreamConfig.width,
                               StreamConfig.height, StreamConfig.fps,
                               rendererContext, drFlags);
    if (err != 0) {
        return err;
    }

    VideoCallbacks.start();
    if ((VideoCallbacks.capabilities &
            (CAPABILITY_DIRECT_SUBMIT | CAPABILITY_PULL_RENDERER)) == 0) {
        err = PltCreateThread("VideoDec", VideoDecoderThreadProc, NULL,
                              &decoderThread);
        if (err != 0) {
            VideoCallbacks.stop();
            VideoCallbacks.cleanup();
            return err;
        }
    }

    return 0;
}

const RTP_VIDEO_STATS* LiGetRTPVideoStats(void) {
    return &nativeVideoStats;
}
