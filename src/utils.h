// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_UTILS_H
#define LIBAV_TEST_UTILS_H
#include <iomanip>
#include <iostream>
#include <sstream>
#include <chrono>
#include <uuid/uuid.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <iostream>
using namespace std;

string getFileName()
{
    stringstream ss;
    ss << time(nullptr) << ".ts";
    return ss.str();
}

static void pgm_save(unsigned char * buf, int wrap, int xsize, int ysize, char * filename)
{
    FILE * f;
    int i;

    f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

/**
 * Get now date RFC5322
 * @return string  Sun, 27 Mar 2022 23:20:22 +0500
 */
string getDateRFC5322()
{
    time_t current = time(nullptr);
    std::tm tm = *std::localtime(&current);
    stringstream ss;
    ss.imbue(std::locale("en_US.UTF-8"));
    ss << put_time(&tm, "%a, %d %b %Y %T %z");
    return ss.str();
}

string uuid()
{
    /**
     * Make uuid
     */
    uuid_t out;
    uuid_generate(out);
    char uuid_str[37];
    uuid_unparse_lower(out, uuid_str);
    stringstream ss;
    ss << uuid_str;
    return ss.str();
}

string getISOCurrentTimestamp()
{
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    stringstream ss;
    ss << std::put_time(std::localtime(&t), "%FT%T%z");
    return ss.str();
}
#endif // LIBAV_TEST_UTILS_H
