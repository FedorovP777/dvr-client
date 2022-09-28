// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_INPUTSTREAM_H
#define LIBAV_TEST_INPUTSTREAM_H

#include <memory>
#include <set>
#include <string>
#include <vector>
#include "Context.h"
#include "Options.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libavutil/common.h>
#include <libavutil/timestamp.h>
}
using namespace std;

class InputStream
{
public:
    shared_ptr<Context> context;
    shared_ptr<Options> options;
    //    AVCodecContext * codec_decode_context;
    AVPixelFormat pixel_format;
    int video_stream_index = 0;
    int audio_stream_index = 0;
    int64_t latest_video_pts = 0;
    //    AVFrame * decode_frame;
    vector<int> stream_mapping;
    string source;
    vector<AVCodecContext *> decode_contexts;
    InputStreamSetting * input_stream_setting;
    explicit InputStream(InputStreamSetting * input_stream_setting_)
        : pixel_format(AV_PIX_FMT_YUV420P), input_stream_setting(input_stream_setting_)
    {
        Logger l(std::cout);
        this->context = std::make_shared<Context>();
        //        decode_frame = av_frame_alloc();
        source = input_stream_setting->src;
        this->options = input_stream_setting->options;
        //        if (!decode_frame)
        //        {
        //            fprintf(stderr, "Could not allocate video decode_frame\n");
        //            exit(1);
        //        }

        LOG("InputStream constructor")
    }


    void createDecodeContext(AVCodecID codec_id, int stream_id)
    {
        auto * codec = avcodec_find_decoder(codec_id);

        if (codec == nullptr)
            exit(1);

        auto * codec_decode_context = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_decode_context, context->cntx->streams[stream_id]->codecpar);

        if (avcodec_open2(codec_decode_context, codec, nullptr) < 0)
            exit(1);

        decode_contexts[stream_id] = codec_decode_context;
    }
    ~InputStream()
    {
        Logger l(std::cout);
        LOG("Input stream destructor")
        avformat_close_input(&context->cntx);
        //        av_frame_free(&decode_frame);
    }

    void readStreamInfo()
    {
        AVFormatContext * context_ptr = context->cntx;
        context_ptr->flags |= AVFMT_FLAG_GENPTS;
        context_ptr->flags |= AVFMT_FLAG_IGNDTS;

        if (avformat_open_input(&context_ptr, source.c_str(), nullptr, &options->format_opts) < 0)
        {
            fprintf(stderr, "Could not open input file '%s'", source.c_str());
        }
        if (avformat_find_stream_info(context_ptr, nullptr) < 0)
        {
            fprintf(stderr, "Failed to retrieve input stream information");
        }

        av_dump_format(context_ptr, 0, source.c_str(), 0);

        this->decode_contexts.resize(context_ptr->nb_streams);
        for (int i = 0; i < context_ptr->nb_streams; i++)
        {
            if (context_ptr->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                this->createDecodeContext(context_ptr->streams[i]->codecpar->codec_id, i);
                audio_stream_index = i;
                continue;
            }

            if (context_ptr->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                this->createDecodeContext(context_ptr->streams[i]->codecpar->codec_id, i);
                video_stream_index = i;
                continue;
            }
        }

        this->pixel_format = static_cast<AVPixelFormat>(context_ptr->streams[this->video_stream_index]->codecpar->format);
        auto * video_decode_context = this->decode_contexts[video_stream_index];
        video_decode_context->pix_fmt = this->pixel_format;
        video_decode_context->width = context_ptr->streams[this->video_stream_index]->codecpar->width;
        video_decode_context->height = context_ptr->streams[this->video_stream_index]->codecpar->height;

        //        decode_frame->format = video_decode_context->pix_fmt;
        //        decode_frame->width = video_decode_context->width;
        //        decode_frame->height = video_decode_context->height;
        const AVDictionaryEntry * tag = nullptr;

        while ((tag = av_dict_get(context_ptr->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
            printf("%s=1%s\n", tag->key, tag->value);
    }

    bool isVideoPaket(AVPacket * paket) const { return paket->stream_index == this->video_stream_index; }
    bool isAudioPaket(AVPacket * paket) const { return paket->stream_index == this->audio_stream_index; }

    void setVideoLatestPTS(int64_t pts) { this->latest_video_pts = pts; }
    int64_t getVideoLatestPTS() const { return this->latest_video_pts; }
    [[nodiscard]] float getWidthHeightRelation() const
    {
        return static_cast<float>(this->context->cntx->streams[this->video_stream_index]->codecpar->width)
            / static_cast<float>(this->context->cntx->streams[this->video_stream_index]->codecpar->height);
    }

    int calculateWidth(int height) const
    {
        int width = ceil(height * this->getWidthHeightRelation());
        if (width % 2 != 0)
        {
            width++;
        }
        return width;
    }

    [[nodiscard]] vector<int> getMapStreams()
    {
        set<int> allow_codec_type = {AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO};
        AVFormatContext * context_ptr = context->cntx;
        for (int i = 0; i < context_ptr->nb_streams; i++)
        {
            if (allow_codec_type.contains(context_ptr->streams[i]->codecpar->codec_type))
            {
                stream_mapping.push_back(i);
            }
        }
        return stream_mapping;
    }

    template <typename F>
    void decodePacketLambda(AVPacket * pkt, const F & f)
    {
        auto * decode_context = this->decode_contexts[pkt->stream_index];
        if (auto ret = avcodec_send_packet(decode_context, pkt) < 0)
        {
            char err[1024] = {0};
            av_strerror(ret, err, 1024);
            cout << err << endl;
            cout << pkt->stream_index << endl;
            fprintf(stderr, "Error sending a packet for decoding %d\n", ret);
            return;
        }
        int ret = 0;
        while (ret >= 0)
        {
            AVFrame * decode_frame = av_frame_alloc();
            ret = avcodec_receive_frame(decode_context, decode_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            else if (ret < 0)
            {
                fprintf(stderr, "Error during decoding\n");
                exit(1);
            }


            f(decode_frame);
        }
    }
};

#endif // LIBAV_TEST_INPUTSTREAM_H
