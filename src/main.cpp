// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com


#include <uv.h>
#include <fmt/core.h>
#include "ConfigService.h"
#include "Context.h"
#include "EventService.h"
#include "FormatFilename.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "Packet.h"
#include "SharedQueue.h"
#include "StreamWorker.h"
#include "StreamHealthChecker.h"
#include <chrono>
using namespace std;
using namespace std::chrono;

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        LOG("Invalid argument")
        return 1;
    }
    auto start = std::chrono::steady_clock::now();
    ConfigService config_service;
    std::string yaml_path = argv[1];
    config_service.loadConfigFile(yaml_path);
    auto stream_config = config_service.getStreams();
    auto stream_workers = vector<StreamWorker *>();
    for (auto & v : stream_config)
    {
        stream_workers.emplace_back(new StreamWorker(v));
        stream_workers.back()->run();
    }
    EventService::registerHealthCheckJob<timer_health_check_struct, vector<StreamWorker *>, StreamHealthChecker>(
        stream_workers, new StreamHealthChecker);


    for (auto & stream_worker : stream_workers)
    {
        EventService::registerWriteJob<job_writer_struct *>(new job_writer_struct{stream_worker});
    }

    EventService::registerReloadSignal();
    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
