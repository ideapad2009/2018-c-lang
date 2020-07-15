#ifndef _OPUS_DECODER_H_
#define _OPUS_DECODER_H_
#include "SoftCodec.h"
#include "opus.h"
#include <ogg/ogg.h>

#include "opus_multistream.h"
#include "opus_header.h"

typedef struct {
    int _run;

    ////opus global variable
    ogg_sync_state oy;
    ogg_page       og;
    ogg_packet     op;
    ogg_stream_state os;
    int eos;
    opus_int64 packet_count;
    int frame_size;
    int stream_init;
    opus_int16 *output;

    ogg_int64_t page_granule;
    int has_opus_stream;
    int has_tags_packet;
    OpusMSDecoder *st;
    ogg_int32_t opus_serialno;
    int streams;
    int channels;
    int rate;

    OpusHeader header;

    opus_int64 pause_packet_offset;
    opus_int64 page_packet_offset;

    ////debug value
    opus_int64 opusframecnt;
    opus_int64 opuspcmcnt;
    opus_int64 opuspagepos;
    opus_int64 readbytes;

} OpusDecCodec;

int OpusOpen(SoftCodec *softCodec);
int OpusClose(SoftCodec *softCodec);
int OpusProcess(SoftCodec *softCodec);
#endif
