// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef CLANG_TIDY_SRC_STREAMHEALTHCHECKER_H_
#define CLANG_TIDY_SRC_STREAMHEALTHCHECKER_H_


template <
    class result_t = std::chrono::milliseconds,
    class clock_t = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds>
auto since(std::chrono::time_point<clock_t, duration_t> const & start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}


class StreamHealthChecker
{
public:
    StreamHealthChecker() { start_time = std::chrono::steady_clock::now(); }
    void checkStreams(vector<StreamWorker *> * stream_workers)
    {
        int idx = 0;

        for (auto & stream : *stream_workers)
        {
            if (pts_values.contains(idx))
            {
                LOG("Previous PTS: " << pts_values[idx].second << " Current PTS: " << stream->latest_write_paket_pts
                                     << " Diff: " << stream->latest_write_paket_pts - pts_values[idx].second
                                     << "ms QueueWriteSize:" << stream->getWriteQueueSize())
                if (pts_values[idx].second == stream->latest_write_paket_pts)
                {
                    LOG("Freeze stream detected!")
                    exit(0);
                }
                if (stream->getWriteQueueSize() > 1000)
                {
                    LOG("Problem with write to disk!")
                    exit(0);
                }
            }
            pts_values[idx] = std::pair<int, int64_t>{since(start_time).count(), stream->latest_write_paket_pts};
            ++idx;
        }
        //        cout << since(start_time).count() << endl;
        //        previous_run_check = std::chrono::steady_clock::now();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::time_point<std::chrono::steady_clock> previous_run_check;
    std::unordered_map<int, pair<int, int64_t>> pts_values;
};

struct timer_health_check_struct
{
    StreamHealthChecker * stream_health_checker;
    vector<StreamWorker *> * stream_workers;
};

struct job_writer_struct
{
    StreamWorker * stream_worker;
    uv_timer_t uv_timer;
    uv_work_t uv_work;
    uv_async_t uv_async;
};
#endif //CLANG_TIDY_SRC_STREAMHEALTHCHECKER_H_
