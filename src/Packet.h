// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_PACKET_H
#define LIBAV_TEST_PACKET_H

#include <iostream>
#include "Logger.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class Packet
{
public:
    AVPacket * pkt;

    Packet()
    {
        Logger l(std::cout);
        LOG("Packet:constructor")
        this->pkt = av_packet_alloc();
        if (!this->pkt)
        {
            fprintf(stderr, "Could not allocate AVPacket\n");
        }
    }

    ~Packet()
    {
        Logger l(std::cout);
        LOG("Packet:destructor")
        av_packet_free(&pkt);
    }
};

#endif // LIBAV_TEST_PACKET_H
