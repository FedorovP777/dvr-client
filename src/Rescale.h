// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_RESCALE_H
#define LIBAV_TEST_RESCALE_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
using namespace std;

class Rescale
{
private:
    SwsContext * sws_ctx;
    int dstWidth;
    int dstHeight;

public:
    Rescale(AVCodecContext *& codecContext, int dstWidth, int dstHeight)
    {
        this->dstWidth = dstWidth;
        this->dstHeight = dstHeight;
        sws_ctx = sws_getContext(
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,
            this->dstWidth,
            this->dstHeight,
            AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);

        if (!sws_ctx)
        {
            fprintf(stderr, "Impossible to create scale codecContext for the conversion \n");
            exit(1);
        }
    }

    void rescale(const AVFrame * frameIn, AVFrame * frameOut)
    {
        sws_scale(
            sws_ctx, (const uint8_t * const *)frameIn->data, frameIn->linesize, 0, frameIn->height, frameOut->data, frameOut->linesize);
        frameOut->width = this->dstWidth;
        frameOut->height = this->dstHeight;
    }

    ~Rescale() { sws_freeContext(sws_ctx); }
};

#endif // LIBAV_TEST_RESCALE_H
