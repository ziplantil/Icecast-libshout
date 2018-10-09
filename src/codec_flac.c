/* -*- c-basic-offset: 8; -*- */
/* flac.c: Ogg FLAC data handlers for libshout
 * $Id$
 *
 *  Copyright (C) 2002-2004 the Icecast team <team@icecast.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <stdlib.h>

#include "FLAC/stream_decoder.h"

#include "shout_private.h"
#include "format_ogg.h"

/* -- local data structures -- */
typedef struct {
    FLAC__StreamDecoder *dec;
    uint64_t samplesInPage;
    long packetStartIndex;
    char err;
    char hasPacket;
    ogg_packet packet;
} flac_data_t;

/* -- local prototypes -- */
static int read_flac_page(ogg_codec_t *codec, ogg_page *page);
static void free_flac_data(void *codec_data);
FLAC__StreamDecoderReadStatus flac_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
FLAC__StreamDecoderWriteStatus flac_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data);
void flac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

/* -- flac functions -- */
int _shout_open_flac(ogg_codec_t *codec, ogg_page *page)
{
    flac_data_t *flac_data = calloc(1, sizeof(flac_data_t));
    FLAC__StreamDecoder *decoder = NULL;
    ogg_packet packet;

    (void)page;

    if (!flac_data)
        return SHOUTERR_MALLOC;

    if ((decoder = FLAC__stream_decoder_new()) == NULL) {
        return SHOUTERR_UNSUPPORTED;
    }

    if (FLAC__STREAM_DECODER_INIT_STATUS_OK != FLAC__stream_decoder_init_stream(decoder, flac_read_callback, NULL, NULL, NULL, NULL, flac_write_callback, NULL, flac_error_callback, flac_data)) {
        free_flac_data(flac_data);

        return SHOUTERR_UNSUPPORTED;
    }

    ogg_stream_packetout(&codec->os, &packet);

    flac_data->dec = decoder;
    flac_data->err = 0;
    flac_data->hasPacket = 1;
    flac_data->packet = packet;
    flac_data->packetStartIndex = 9L;

    if (!FLAC__stream_decoder_process_single(decoder)) {
        free_flac_data(flac_data);

        return SHOUTERR_UNSUPPORTED;
    }

    codec->codec_data = flac_data;
    codec->read_page = read_flac_page;
    codec->free_data = free_flac_data;

    return SHOUTERR_SUCCESS;
}

static int read_flac_page(ogg_codec_t *codec, ogg_page *page)
{
    flac_data_t *flac_data = codec->codec_data;
    ogg_packet packet;
    unsigned rate;
    char is_page_eos;

    (void)page;

    is_page_eos = ogg_page_eos(page);

    if (flac_data->err)
        return SHOUTERR_INSANE;

    flac_data->samplesInPage = 0;

    while (ogg_stream_packetout(&codec->os, &packet) > 0)
    {
        flac_data->hasPacket = 1;
        flac_data->packet = packet;
        flac_data->packetStartIndex = 0;
        if (!FLAC__stream_decoder_process_single(flac_data->dec))
            return SHOUTERR_INSANE;
    }

    rate = FLAC__stream_decoder_get_sample_rate(flac_data->dec);
    if (rate && !is_page_eos) {
        codec->senttime += ((flac_data->samplesInPage * 1000000) / rate);
    }

    return SHOUTERR_SUCCESS;
}

static void free_flac_data(void *codec_data)
{
    flac_data_t *flac_data = (flac_data_t *)codec_data;

    if (flac_data->dec) {
        FLAC__stream_decoder_finish(flac_data->dec);
        FLAC__stream_decoder_delete(flac_data->dec);
    }

    free(flac_data);
}

FLAC__StreamDecoderReadStatus flac_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    flac_data_t *flac_data = client_data;
    ogg_packet packet;

    if (flac_data->hasPacket) {
        unsigned char *packet_data;
        long remaining;

        /* give rest of packet */
        packet = flac_data->packet;
        packet_data = packet.packet + flac_data->packetStartIndex;
        remaining = packet.bytes - flac_data->packetStartIndex;

        if (remaining > *bytes) {
            memcpy(buffer, packet_data, *bytes);
            flac_data->packetStartIndex += *bytes;
        } else {
            memcpy(buffer, packet_data, remaining);
            *bytes = (size_t) remaining;
            flac_data->hasPacket = 0;
            flac_data->packetStartIndex = 0L;
        }

        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }

    *bytes = 0;
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

FLAC__StreamDecoderWriteStatus flac_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
    flac_data_t *flac_data = client_data;
    flac_data->samplesInPage += frame->header.blocksize;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    flac_data_t *flac_data = client_data;
    flac_data->err = 1;
}
