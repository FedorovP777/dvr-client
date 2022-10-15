// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef MY_PROJECT_SRC_EVENTSERVICE_H_
#define MY_PROJECT_SRC_EVENTSERVICE_H_
#include <future>
#include <uv.h>
#include "ConfigService.h"
#include "S3Service.h"
#include "NotificationService.h"
struct UploadToS3Task
{
    string filename;
    string current_server_name;
    string filename_template;
    string camera_name;
    string dest;
    string dest_name;
    S3Profile * s3_profile = nullptr;
    NotificationEndpoint * notification_endpoint = nullptr;
};


class EventService
{
public:
    static void afterUploadFile(uv_work_t * req, int /*status*/)
    {
        auto * upload_to_s3task = static_cast<UploadToS3Task *>(req->data);
        delete upload_to_s3task;
        delete req;
    }

    static void uploadFile(uv_work_t * req)
    {
        auto * task = static_cast<UploadToS3Task *>(req->data);
        LOG("Start upload"
            << "thread:" << std::this_thread::get_id() << endl
            << "filename" << task->filename)
        auto * notification_context = new NotificationContext;
        notification_context->result_upload_file = S3Service::putObject(task->filename, task->s3_profile);
        notification_context->s3_profile = task->s3_profile;
        notification_context->dest_name = task->dest_name;
        notification_context->camera_name = task->camera_name;
        notification_context->notification_endpoint = task->notification_endpoint;
        cout << task->notification_endpoint->url << endl;
        sendAsync<NotificationContext *, void (*)(uv_async_s *)>(notification_context, sendNotifyAfterUploadAsync);
        LOG("Upload done.")
    }


    static void finishWriteFragmentAsync(uv_async_t * msg)
    {
        auto * task = static_cast<UploadToS3Task *>(msg->data);
        LOG("finishWriteFragmentAsync" << task->filename)
        uv_close(reinterpret_cast<uv_handle_t *>(msg), (uv_close_cb)free);
        auto * req = new uv_work_t;
        req->data = reinterpret_cast<void *>(task);
        uv_queue_work(uv_default_loop(), req, EventService::uploadFile, EventService::afterUploadFile);
    }
    static void signalHandler(uv_signal_t * /*req*/, int /*signum*/)
    {
        cout << "Signal received!" << endl;
        //TODO (me): update config by signal
    }

    static void registerReloadSignal()
    {
        uv_signal_t sig;
        uv_signal_init(uv_default_loop(), &sig);
        uv_signal_start(&sig, signalHandler, SIGUSR1);
    }

    template <typename T>
    static void writeJob(uv_work_t * req)
    {
        T data = static_cast<T>(req->data);
        data->stream_worker->processingOnePacketFromQueue();
    }

    template <typename T1, typename T2>
    static void sendAsync(T1 data, T2 fn)
    {
        auto * async = new uv_async_t;
        async->data = (void *)data;
        uv_async_init(uv_default_loop(), async, fn);
        uv_async_send(async);
    }
    template <typename T>
    static void startWriteJob(uv_async_t * handle)
    {
        uv_close(reinterpret_cast<uv_handle_t *>(handle), NULL);
        T data = static_cast<T>(handle->data);

        int timeout = 500;
        if (data->stream_worker->getWriteQueueSize() > 0)
        {
            timeout = 0;
        }
        uv_timer_stop(&data->uv_timer);
        uv_timer_init(uv_default_loop(), &data->uv_timer);
        uv_timer_start(
            &data->uv_timer,
            [](uv_timer_t * handle_timer) {
                T data = static_cast<T>(handle_timer->data);
                uv_queue_work(uv_default_loop(), &data->uv_work, writeJob<T>, [](uv_work_t * req, int status) {
                    T data = static_cast<T>(req->data);
                    uv_async_init(uv_default_loop(), &data->uv_async, startWriteJob<T>);
                    uv_async_send(&data->uv_async);
                });
            },
            timeout,
            0);
    }

    /**
     * Periodic health check task
     *
     * @param handle
     */
    template <typename T>
    static void timerWriterJob(uv_timer_t * handle)
    {
        EventService::sendAsync<void *, void (*)(uv_async_s *)>(handle->data, startWriteJob<T>);
    }

    /**
     * Periodic health check task
     *
     * @param handle
     */
    template <typename T>
    static void streamHealthCheckerJob(uv_timer_t * handle)
    {
        LOG("Start stream health checker job")
        T * data = static_cast<T *>(handle->data);
        data->stream_health_checker->checkStreams(data->stream_workers);
        LOG("Stream health checker job done")
    }
    template <typename T1>
    static void registerWriteJob(T1 job_writer_struct)
    {
        job_writer_struct->uv_timer.data = job_writer_struct;
        job_writer_struct->uv_work.data = job_writer_struct;
        job_writer_struct->uv_async.data = job_writer_struct;
        uv_timer_init(uv_default_loop(), &job_writer_struct->uv_timer);

        const int TIMER_JOB_TIMEOUT_MS = 10 * 100;
        const int TIMER_JOB_INTERVAL_MS = 0;
        uv_timer_start(&job_writer_struct->uv_timer, EventService::timerWriterJob<T1>, TIMER_JOB_TIMEOUT_MS, TIMER_JOB_INTERVAL_MS);
    }
    template <typename T1, typename T2, typename T3>
    static void registerHealthCheckJob(T2 & stream_workers, T3 * stream_health_checker)
    {
        auto * timer_req = new uv_timer_t;
        uv_timer_init(uv_default_loop(), timer_req);
        timer_req->data = new T1{stream_health_checker, &stream_workers};
        const int HEALTH_CHECK_JOB_TIMEOUT_MS = 10 * 1000;
        const int HEALTH_CHECK_JOB_INTERVAL_MS = 10 * 1000;

        uv_timer_start(timer_req, EventService::streamHealthCheckerJob<T1>, HEALTH_CHECK_JOB_TIMEOUT_MS, HEALTH_CHECK_JOB_INTERVAL_MS);
    }
    static void sendNotifyAfterUploadAsync(uv_async_t * task)
    {
        auto * context = reinterpret_cast<NotificationContext *>(task->data);
        auto * req = new uv_work_t;
        req->data = reinterpret_cast<void *>(task->data);
        uv_close(reinterpret_cast<uv_handle_t *>(task), (uv_close_cb)free);
        uv_queue_work(
            uv_default_loop(),
            req,
            [](uv_work_t * req) {
                auto * context = reinterpret_cast<NotificationContext *>(req->data);
                NotificationService::successUploadFile(context);
            },
            [](uv_work_t * req, int status) {
                auto * context = reinterpret_cast<NotificationContext *>(req->data);
                delete context;
                free(req);
            });
    }
};

#endif // MY_PROJECT_SRC_EVENTSERVICE_H_
