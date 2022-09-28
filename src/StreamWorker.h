// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef MY_PROJECT_SRC_STREAMWORKER_H_
#define MY_PROJECT_SRC_STREAMWORKER_H_
#include <future>
using namespace std;

class StreamWorker
{
private:
    StreamSetting stream_setting;
    atomic<bool> exit_flag = false;
    shared_ptr<future<void>> receive_thread;
    shared_ptr<future<void>> write_thread;
    shared_ptr<Packet> input_packet;
    shared_ptr<InputStream> input_stream;
    vector<unique_ptr<OutputStream>> os;
    SharedQueue<AVPacket *> queue_write;
    vector<int> map_streams;
    bool is_need_transcode = false;

public:
    atomic_int64_t latest_receive_paket_pts;
    atomic_int64_t latest_write_paket_pts;

    int getWriteQueueSize() { return this->queue_write.getSize(); }

    explicit StreamWorker(StreamSetting _ss) : stream_setting(_ss)
    {
        LOG("StreamWorker constructor")
        input_packet = make_shared<Packet>();
        input_stream = make_shared<InputStream>(&stream_setting.input_stream_setting);
    }
    void stop()
    {
        LOG("StreamWorker stop")
        exit_flag = true;
        receive_thread->wait();
        write_thread->wait();
        LOG("StreamWorker stop done")
    }
    void run()
    {
        input_stream->readStreamInfo();
        map_streams = input_stream->getMapStreams();
        //Исходящий поток

        for (auto & output_streams_config : stream_setting.output_stream_settings)
        {
            string out_filename_template = output_streams_config.path;
            int width = input_stream->calculateWidth(output_streams_config.height);

            os.push_back(move(make_unique<OutputStream>(
                out_filename_template,
                map_streams,
                input_stream->context,
                input_stream->decode_contexts[input_stream->video_stream_index],
                width,
                output_streams_config.height,
                output_streams_config,
                input_stream->input_stream_setting->name)));
            os.back()->updateOutputFilename();
        }

        for (auto & i : os)
        {
            if (i->is_transcoding)
            {
                is_need_transcode = true;
            }
        }
        //                    {
        //Запуск
        this->receive_thread = make_shared<std::future<void>>(std::async(std::launch::async, [this]() {
            while (true)
            {
                if (exit_flag)
                {
                    queue_write.justNotify();
                    return;
                }
                if (av_read_frame(input_stream->context->cntx, input_packet->pkt) < 0)
                    break;


                if (input_packet->pkt->pts < 1)
                {
                    LOG("PTS < 1")
                    av_packet_unref(input_packet->pkt);
                    continue;
                }

                if (input_packet->pkt->size <= 0)
                {
                    LOG("Size < 0")
                    av_packet_unref(input_packet->pkt);
                    continue;
                }
                auto * pkt_clone = av_packet_clone(input_packet->pkt);
                av_packet_unref(input_packet->pkt);
                latest_receive_paket_pts = pkt_clone->pts;
                queue_write.push(pkt_clone);
            }
        }));
    }
    void processingOnePacketFromQueue()
    {
        if (exit_flag)
            return;

        auto * pkt = queue_write.popIfExist();

        if (exit_flag || pkt == nullptr)
            return;


        if (input_stream->isVideoPaket(pkt))
        {
            input_stream->setVideoLatestPTS(pkt->pts);
        }
        if (input_stream->isAudioPaket(pkt))
        {
            pkt->pts = input_stream->getVideoLatestPTS();
            pkt->dts = input_stream->getVideoLatestPTS() - 1;
        }
        else
        {
            pkt->dts = AV_NOPTS_VALUE;
        }

        if (is_need_transcode)
        {
            input_stream->decodePacketLambda(pkt, [this, &pkt](AVFrame * frame) {
                for (auto & i : os)
                {
                    if (i->is_transcoding)
                    {
                        if (input_stream->isVideoPaket(pkt))
                        {
                            i->rescaleEncodeWrite(frame);
                        }
                        else
                        {
                            i->writeFrame(pkt);
                        }
                    }
                }
                av_frame_free(&frame);
            });
        }


        for (auto & i : os)
        {
            if (!i->is_transcoding)
            {
                i->writeFrame(pkt);
            }
            if (input_stream->isVideoPaket(pkt) && i->isNeedUpdateFilename(pkt->pts))
            {
                i->updateOutputFilename();
            }
        }
        latest_write_paket_pts = pkt->pts;
        av_packet_free(&pkt);
        delete pkt;
    }
};
#endif // MY_PROJECT_SRC_STREAMWORKER_H_