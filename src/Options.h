// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_OPTIONS_H
#define LIBAV_TEST_OPTIONS_H

#include "Logger.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/common.h>
#include <libavutil/timestamp.h>
}

#include <iostream>

class Options
{
public:
    AVDictionary * format_opts;

    Options()
    {
        Logger l(std::cout);
        LOG("Options:constructor")
        format_opts = NULL;
    }

    void set(const char * key, const char * value) { av_dict_set(&format_opts, key, value, 0); }

    ~Options()
    {
        Logger l(std::cout);
        LOG("Options:destructor")
        av_dict_free(&format_opts);
    }
};

#endif // LIBAV_TEST_OPTIONS_H
