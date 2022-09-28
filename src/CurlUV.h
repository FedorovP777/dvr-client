//
// Created by User on 03.07.2022.
//

#ifndef CLANG_TIDY_SRC_CURLUV_H_
#define CLANG_TIDY_SRC_CURLUV_H_
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>

class CurlUV
{
public:
    struct HandleSocketData
    {
        CURLM * curl_handle;
        std::function<void()> done_cb;
        ~HandleSocketData() { cout << "DESCTRUCT" << endl; }
    };
    struct TimerRequest
    {
        uv_timer_t uv_timer;
        CURLM * curl_handle;
        int curl_socket_action_running;
        std::function<void()> done_cb;
    };

    struct CurlContext
    {
        uv_poll_t poll_handle;
        CURLM * curl_handle;
        curl_socket_t sockfd;
        std::function<void()> done_cb;
        HandleSocketData * handle_socket_data;
    };


    static CurlContext * createCurlContext(curl_socket_t sockfd, HandleSocketData * hsd)
    {
        cout << "createCurlContext" << endl;
        auto * context = new CurlContext;
        context->sockfd = sockfd;
        context->curl_handle = hsd->curl_handle;
        context->done_cb = hsd->done_cb;
        context->handle_socket_data = hsd;
        context->poll_handle.data = context;

        return context;
    }

    static void curlCloseCb(uv_handle_t * handle) { delete reinterpret_cast<CurlContext *>(handle->data); }

    static void addDownload(const char * url, CURLM * curl_handle)
    {
        char filename[50];
        FILE * file;
        CURL * handle;

        snprintf(filename, 50, "./%s.download", "test11");

        file = fopen(filename, "wb");
        if (!file)
        {
            fprintf(stderr, "Error opening %s\n", filename);
            return;
        }

        handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(handle, CURLOPT_PRIVATE, file);
        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_multi_add_handle(curl_handle, handle);
        fprintf(stderr, "Added download %s -> %s\n", url, filename);
    }

    static void checkMultiInfo(CURLM * curl_handle, std::function<void()> done_cb, HandleSocketData * hsd)
    {
        CURLMsg * message;
        int pending;
        CURL * easy_handle;
        FILE * file;

        while ((message = curl_multi_info_read(curl_handle, &pending)))
        {
            switch (message->msg)
            {
                case CURLMSG_DONE:
                    /* Do not use message data after calling curl_multi_remove_handle() and
                       curl_easy_cleanup(). As per curl_multi_info_read() docs:
                       "WARNING: The data the returned pointer points to will not survive
                       calling curl_multi_cleanup, curl_multi_remove_handle or
                       curl_easy_cleanup." */
                    easy_handle = message->easy_handle;
                    char * done_url;
                    curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
                    curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &file);

                    curl_multi_remove_handle(curl_handle, easy_handle);
                    curl_easy_cleanup(easy_handle);
                    if (file)
                    {
                        fclose(file);
                    }
                    if (hsd != nullptr)
                    {
                        delete hsd;
                    }
                    done_cb();
                    break;

                default:
                    fprintf(stderr, "CURLMSG default\n");
                    break;
            }
        }
    }

    static void curlEventPerform(uv_poll_t * req, int /*status*/, int events)
    {
        cout << "curlEventPerform" << endl;

        int running_handles;
        int flags = 0;
        CurlContext * context;

        if (events & UV_READABLE)
            flags |= CURL_CSELECT_IN;
        if (events & UV_WRITABLE)
            flags |= CURL_CSELECT_OUT;

        context = static_cast<CurlContext *>(req->data);
        curl_multi_socket_action(context->curl_handle, context->sockfd, flags, &running_handles);
        checkMultiInfo(context->curl_handle, context->done_cb, context->handle_socket_data);
    }

    static void onTimeout(uv_timer_t * req)
    {
        auto * timer_req = reinterpret_cast<TimerRequest *>(req->data);
        curl_multi_socket_action(timer_req->curl_handle, CURL_SOCKET_TIMEOUT, 0, &timer_req->curl_socket_action_running);
        checkMultiInfo(timer_req->curl_handle, timer_req->done_cb, nullptr);
    }

    static int startTimeout(CURLM * multi, long timeout_ms, void * userp)
    {
        auto * timer_req = reinterpret_cast<TimerRequest *>(userp);
        cout << "timeout_ms:" << timeout_ms << " " << &timer_req->uv_timer << endl;
        if (timeout_ms < 0)
        {
            uv_timer_stop(&timer_req->uv_timer);
            uv_close((uv_handle_t *)&timer_req->uv_timer, [](uv_handle_t * handle) {
                auto * timer_req = reinterpret_cast<TimerRequest *>(handle);
                delete timer_req;
            });
        }
        else
        {
            if (timeout_ms == 0)
            {
                timeout_ms = 1; /* 0 means directly call socket_action, but we will do it in a bit */
            }
            uv_timer_start(&timer_req->uv_timer, onTimeout, timeout_ms, 0);
        }
        return 0;
    }


    static int handleSocket(CURL * easy, curl_socket_t curl_socket_type, int action, void * userp, void * socketp)
    {
        auto * handle_socket_data = reinterpret_cast<HandleSocketData *>(userp);
        auto * curl_context = reinterpret_cast<CurlContext *>(socketp);
        int events = 0;

        switch (action)
        {
            case CURL_POLL_IN:
            case CURL_POLL_OUT:
            case CURL_POLL_INOUT:

                if (!socketp)
                {
                    curl_context = createCurlContext(curl_socket_type, handle_socket_data);
                    uv_poll_init_socket(uv_default_loop(), &curl_context->poll_handle, curl_socket_type);
                }
                curl_multi_assign(handle_socket_data->curl_handle, curl_socket_type, reinterpret_cast<void *>(curl_context));

                if (action != CURL_POLL_IN)
                    events |= UV_WRITABLE;
                if (action != CURL_POLL_OUT)
                    events |= UV_READABLE;

                uv_poll_start(&curl_context->poll_handle, events, curlEventPerform);
                break;
            case CURL_POLL_REMOVE:
                cout << "CURL_POLL_REMOVE" << endl;
                if (socketp)
                {
                    uv_poll_stop(&curl_context->poll_handle);
                    uv_close(reinterpret_cast<uv_handle_t *>(&curl_context->poll_handle), curlCloseCb);
                    curl_multi_assign(handle_socket_data->curl_handle, curl_socket_type, NULL);
                }
                break;
            default:
                abort();
        }

        return 0;
    }

    static int request(std::function<void()> good_cb)
    {
        if (curl_global_init(CURL_GLOBAL_ALL))
        {
            fprintf(stderr, "Could not init curl\n");
            return 1;
        }

        auto * timer_req = new TimerRequest;
        CURLM * curl_handle = curl_multi_init();
        timer_req->curl_handle = curl_handle;
        timer_req->done_cb = good_cb;
        timer_req->uv_timer.data = reinterpret_cast<void *>(timer_req);
        uv_timer_init(uv_default_loop(), &timer_req->uv_timer);

        auto * handle_socket_data = new HandleSocketData;
        handle_socket_data->done_cb = good_cb;
        handle_socket_data->curl_handle = curl_handle;
        curl_multi_setopt(curl_handle, CURLMOPT_SOCKETFUNCTION, handleSocket);
        curl_multi_setopt(curl_handle, CURLMOPT_SOCKETDATA, handle_socket_data);
        curl_multi_setopt(curl_handle, CURLMOPT_TIMERFUNCTION, startTimeout);
        curl_multi_setopt(curl_handle, CURLMOPT_TIMERDATA, timer_req);
        addDownload(
            "https://gist.githubusercontent.com/FedorovP777/5d0cdd318b64b07fa21f3ed2ea8098ee/raw/3a75fd678f211233d36eda0d4dbedc6a2712528a/"
            "gistfile1.txt",
            curl_handle);
    }
};

#endif //CLANG_TIDY_SRC_CURLUV_H_
