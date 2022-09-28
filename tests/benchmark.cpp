#include <benchmark/benchmark.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/common.h>
#include <libavutil/timestamp.h>
}
#include "../src/SharedQueue.h"
static void BM_QueuePush(benchmark::State & state)
{
    SharedQueue<AVPacket *> queue_write;
    for (auto _ : state)
    {
        auto * p = new AVPacket;
        queue_write.push(p);
    }
}
BENCHMARK(BM_QueuePush);

static void BM_QueuePushNoVariable(benchmark::State & state)
{
    SharedQueue<AVPacket *> queue_write;
    for (auto _ : state)
    {
        queue_write.push(new AVPacket);
    }
}
BENCHMARK(BM_QueuePushNoVariable);

static void BM_QueuePushNoVariableNoMutex(benchmark::State & state)
{
    SharedQueue<AVPacket *> queue_write;
    queue_write.use_mutex = false;
    for (auto _ : state)
    {
        queue_write.push(new AVPacket);
    }
}
BENCHMARK(BM_QueuePushNoVariableNoMutex);

static void BM_QueuePushNoNewObject(benchmark::State & state)
{
    SharedQueue<AVPacket *> queue_write;
    AVPacket p;
    for (auto _ : state)
    {
        queue_write.push(&p);
    }
}
BENCHMARK(BM_QueuePushNoNewObject);

BENCHMARK_MAIN();