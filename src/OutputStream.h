// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_OUTPUTSTREAM_H
#define LIBAV_TEST_OUTPUTSTREAM_H

#include <future>
#include <memory>
#include <vector>
#include <uv.h>
#include <filesystem>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/common.h>
#include <libavutil/timestamp.h>
}

#include "ConfigService.h"
#include "Context.h"
#include "FormatFilename.h"
#include "Rescale.h"
#include "S3Service.h"

namespace fs = std::filesystem;
class OutputStream
{
public:
    unique_ptr<Rescale> rescale;
    unique_ptr<Context> output_context;
    string dest;
    string camera_name;
    AVPacket * pkt_out;
    AVCodec * codec_;
    AVCodecContext * enc_ctx;
    bool needCloseFile = false;
    string outFilenameTemplate;
    bool is_transcoding;
    //    uint16_t bitrate;
    //    optional<int> time_base;
    //    optional<int> gop;
    //    optional<int> max_b_frames;
    //    shared_ptr<Options> encOptions;
    unsigned long long prevIntervalSec = 0;
    unsigned long long currentTimestampSec = 0;
    //    int fileDurationSec = 0;
    string filename;
    //    S3Profile * s3profile;
    OutputStreamSetting * output_stream_setting;
    OutputStream(
        string & outFilenameTemplate_,
        vector<int> map_streams,
        shared_ptr<Context> inputContext,
        AVCodecContext *& codecDecodeContext,
        int width,
        int height,
        OutputStreamSetting & outputStreamSetting,
        string camera_name_)
        : outFilenameTemplate(outFilenameTemplate_)
        , output_stream_setting(&outputStreamSetting)
        , enc_ctx(nullptr)
        , camera_name(camera_name_)
    {
        this->pkt_out = av_packet_alloc();

        if (!this->pkt_out)
            exit(1);
        this->is_transcoding = output_stream_setting->rescale;
        this->output_context = std::make_unique<Context>();
        if (this->is_transcoding)
        {
            this->prepareEncContext(enc_ctx, codec_, width, height);
        }
        LOG("OutputStream constructor")
        this->dest = FormatFilename::formatting(this->outFilenameTemplate, this->camera_name).c_str();
        fs::path p = this->dest;
        cout << p << endl;
        cout << p.parent_path() << endl;
        if (!fs::exists(p.parent_path()))
        {
            fs::create_directory(p.parent_path());
        }
        avformat_alloc_output_context2(&output_context->cntx, NULL, NULL, this->dest.c_str());
        if (!output_context->cntx)
        {
            fprintf(stderr, "Could not create output context\n");
        }

        for (int map_stream : map_streams)
        {
            cout << map_stream << endl;
            AVStream * out_stream;
            out_stream = avformat_new_stream(this->output_context->cntx, NULL);

            if (!out_stream)
            {
                fprintf(stderr, "Failed allocating output stream\n");
                exit(1);
            }

            if (avcodec_parameters_copy(out_stream->codecpar, inputContext->cntx->streams[map_stream]->codecpar) < 0)
            {
                fprintf(stderr, "Failed to copy codec parameters\n");
            }
            out_stream->codecpar->codec_tag = 0;
        }

        av_dump_format(this->output_context->cntx, 0, this->dest.c_str(), 1);
        if (this->is_transcoding)
        {
            this->rescale = make_unique<Rescale>(codecDecodeContext, width, height);
        }
    }

    ~OutputStream()
    {
        Logger l(std::cout);
        LOG("OutputStream destructor")

        LOG("OutputStream av_write_trailer")
        av_write_trailer(this->output_context->cntx);

        if (this->output_context->cntx && !(this->output_context->cntx->flags & AVFMT_NOFILE))
        {
            LOG("OutputStream avio_closep")
            avio_closep(&this->output_context->cntx->pb);
        }
        avcodec_free_context(&enc_ctx);
    }

    void prepareEncContext(AVCodecContext *& enc_ctx, AVCodec *& codec, int width, int height)
    {
        codec = avcodec_find_encoder_by_name("libx264");
        if (!codec)
        {
            fprintf(stderr, "Codec '%s' not found\n", "libx264");
            exit(1);
        }

        enc_ctx = avcodec_alloc_context3(codec);
        if (!enc_ctx)
        {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }

        if (output_stream_setting->bitrate)
        {
            /* put sample parameters */
            enc_ctx->bit_rate = output_stream_setting->bitrate;
        }
        /* resolution must be a multiple of two */
        enc_ctx->width = width;
        enc_ctx->height = height;

        /* frames per second */
        enc_ctx->time_base = (AVRational){1, output_stream_setting->time_base.value_or(25)};

        //    enc_ctx->framerate = (AVRational) {25, 1};
        enc_ctx->flags |= AV_CODEC_FLAG_QSCALE;
        enc_ctx->global_quality = FF_QP2LAMBDA * 25;
        /* emit one intra frame every ten frames
         * check frame pict_type before passing frame
         * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
         * then gop_size is ignored and the output of encoder
         * will always be I frame irrespective to gop_size
         */
        if (output_stream_setting->gop)
        {
            enc_ctx->gop_size = output_stream_setting->gop.value();
        }

        if (output_stream_setting->max_b_frames)
        {
            enc_ctx->max_b_frames = output_stream_setting->max_b_frames.value();
        }

        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

        //    if (codec->id == AV_CODEC_ID_H264)
        //        av_opt_set(&enc_ctx->priv_data, "preset", "slow", 0);

        /* open it */
        if (avcodec_open2(enc_ctx, codec, &output_stream_setting->options->format_opts) < 0)
        {
            fprintf(stderr, "Could not open codec: \n");
            exit(1);
        }
    }

    void updateOutputFilename()
    {
        /**
         * Create new file and upload complete file to s3
         */
        string previous_filename = this->filename;
        this->filename = FormatFilename::formatting(this->outFilenameTemplate, this->camera_name);
        this->setOutputFilename(this->filename);
        this->prevIntervalSec = this->currentTimestampSec;
        LOG("updateOutputFilename")
        if (!previous_filename.empty() && output_stream_setting->s3_profile != nullptr)
        {
            cout << camera_name << "camera_name" << endl;
            auto * async = new uv_async_t;
            auto * task = new UploadToS3Task;
            task->filename = previous_filename;
            task->camera_name = this->camera_name;
            task->dest_name = this->output_stream_setting->name;
            task->dest = this->dest;
            task->s3_profile = output_stream_setting->s3_profile;
            task->notification_endpoint = output_stream_setting->notification_endpoint;
            cout << output_stream_setting->notification_endpoint->url << endl;
            async->data = (void *)task;
            uv_async_init(uv_default_loop(), async, EventService::finishWriteFragmentAsync);
            uv_async_send(async);
        }
    }

    void setOutputFilename(const string & dest_) const
    {
        /**
         * Close current and open new file for write
         */
        av_write_trailer(this->output_context->cntx);
        avio_closep(&this->output_context->cntx->pb);

        if (!(this->output_context->cntx->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&this->output_context->cntx->pb, dest_.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                fprintf(stderr, "Could not open output file '%s'", dest_.c_str());
            }
        }
        if (avformat_write_header(this->output_context->cntx, NULL) < 0)
        {
            fprintf(stderr, "Error occurred when opening output file\n");
        }
    }

    void rescaleEncodeWrite(AVFrame * frame)
    {
        AVFrame * frame_tmp = av_frame_clone(frame);
        av_frame_copy(frame_tmp, frame);
        av_frame_make_writable(frame_tmp);
        this->rescale->rescale(frame, frame_tmp);
        this->encodeAndWrite(frame_tmp);
        av_frame_free(&frame_tmp);
    }

    /**
     * Write a packet to an output media file.
     * @param pkt
     */
    void writeFrame(AVPacket * pkt) const { av_write_frame(this->output_context->cntx, pkt); }

    void encodeAndWrite(const AVFrame * frame) const
    {
        int ret;

        ret = avcodec_send_frame(enc_ctx, frame);
        if (ret < 0)
        {
            fprintf(stderr, "Error sending a frame for encoding\n");
            exit(1);
        }

        while (ret >= 0)
        {
            auto * av_packet_out = av_packet_alloc();
            ret = avcodec_receive_packet(enc_ctx, av_packet_out);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                av_packet_free(&av_packet_out);
                return;
            }
            else if (ret < 0)
            {
                fprintf(stderr, "Error during encoding\n");
                exit(1);
            }
            this->writeFrame(av_packet_out);
            av_packet_free(&av_packet_out);
        }
    }

    bool isNeedUpdateFilename(int64_t & millisecTimestamp)
    {
        currentTimestampSec = millisecTimestamp / 100000;
        return output_stream_setting->file_duration_sec > 0 && currentTimestampSec > 0
            && currentTimestampSec > prevIntervalSec + output_stream_setting->file_duration_sec;
    }
};

#endif // LIBAV_TEST_OUTPUTSTREAM_H
