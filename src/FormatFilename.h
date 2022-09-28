// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_FORMATFILENAME_H
#define LIBAV_TEST_FORMATFILENAME_H

#include <chrono>
#include <ctime>
#include <fmt/core.h>
#include <iomanip>
#include <iostream>
#include <time.h>

class FormatFilename
{
public:
    /**
     * Formatting path file.
     * e.g. /%F/{unixtime}.mp4 -> /2022-05-01/1651398874.mp4
     * @param s
     * @return
     */
    static string formatting(string source, string camera_name)
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto * locale_time = std::localtime(&in_time_t);
        time_t unixtime = time(nullptr);
        std::ostringstream os;
        os << std::put_time(locale_time, source.c_str());
        return fmt::format(os.str(), fmt::arg("unixtime", unixtime), fmt::arg("camera_name", camera_name));
    }
};

#endif // LIBAV_TEST_FORMATFILENAME_H
