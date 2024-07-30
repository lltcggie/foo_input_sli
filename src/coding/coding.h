#ifndef _CODING_H
#define _CODING_H

#include "../vgmstream.h"
#include "../util/reader_sf.h"
#include "../util/log.h"

#ifdef VGM_USE_FFMPEG
/* ffmpeg_decoder */
typedef struct ffmpeg_codec_data ffmpeg_codec_data;

ffmpeg_codec_data* init_ffmpeg_offset(STREAMFILE* sf, uint64_t start, uint64_t size);
ffmpeg_codec_data* init_ffmpeg_header_offset(STREAMFILE* sf, uint8_t* header, uint64_t header_size, uint64_t start, uint64_t size);
ffmpeg_codec_data* init_ffmpeg_header_offset_subsong(STREAMFILE* sf, uint8_t* header, uint64_t header_size, uint64_t start, uint64_t size, int target_subsong);

void decode_ffmpeg(VGMSTREAM* vgmstream, sample_t* outbuf, int32_t samples_to_do, int channels);
void reset_ffmpeg(ffmpeg_codec_data* data);
void seek_ffmpeg(ffmpeg_codec_data* data, int32_t num_sample);
void free_ffmpeg(ffmpeg_codec_data* data);

void ffmpeg_set_skip_samples(ffmpeg_codec_data* data, int skip_samples);
uint32_t ffmpeg_get_channel_layout(ffmpeg_codec_data* data);
void ffmpeg_set_channel_remapping(ffmpeg_codec_data* data, int* channels_remap);
const char* ffmpeg_get_codec_name(ffmpeg_codec_data* data);
void ffmpeg_set_force_seek(ffmpeg_codec_data* data);
void ffmpeg_set_invert_floats(ffmpeg_codec_data* data);
const char* ffmpeg_get_metadata_value(ffmpeg_codec_data* data, const char* key);

int32_t ffmpeg_get_samples(ffmpeg_codec_data* data);
int ffmpeg_get_sample_rate(ffmpeg_codec_data* data);
int ffmpeg_get_channels(ffmpeg_codec_data* data);
int ffmpeg_get_subsong_count(ffmpeg_codec_data* data);

STREAMFILE* ffmpeg_get_streamfile(ffmpeg_codec_data* data);

/* ffmpeg_decoder_utils.c (helper-things) */
ffmpeg_codec_data* init_ffmpeg_atrac3_raw(STREAMFILE* sf, off_t offset, size_t data_size, int sample_count, int channels, int sample_rate, int block_align, int encoder_delay);
ffmpeg_codec_data* init_ffmpeg_atrac3_riff(STREAMFILE* sf, off_t offset, int* out_samples);
ffmpeg_codec_data* init_ffmpeg_atrac3plus_raw(STREAMFILE* sf, uint32_t data_offset, uint32_t data_size, int32_t sample_count, int channels, int sample_rate, int block_align, int encoder_delay);

ffmpeg_codec_data* init_ffmpeg_aac(STREAMFILE* sf, off_t offset, size_t size, int skip_samples);

ffmpeg_codec_data* init_ffmpeg_xwma(STREAMFILE* sf, uint32_t data_offset, uint32_t data_size, int format, int channels, int sample_rate, int avg_bitrate, int block_size);
//TODO: make init_ffmpeg_xwma_fmt(be) too to pass fmt chunk?
ffmpeg_codec_data* init_ffmpeg_xma1_raw(STREAMFILE* sf, uint32_t data_offset, uint32_t data_size, int channels, int sample_rate, int stream_mode);
ffmpeg_codec_data* init_ffmpeg_xma2_raw(STREAMFILE* sf, uint32_t data_offset, uint32_t data_size, int32_t sample_count, int channels, int sample_rate, int block_size, int block_count);
ffmpeg_codec_data* init_ffmpeg_xma_chunk(STREAMFILE* sf, uint32_t data_offset, uint32_t data_size, uint32_t chunk_offset, uint32_t chunk_size);
ffmpeg_codec_data* init_ffmpeg_xma_chunk_split(STREAMFILE* sf_head, STREAMFILE* sf_data, uint32_t data_offset, uint32_t data_size, uint32_t chunk_offset, uint32_t chunk_size);

/* ffmpeg_decoder_custom_opus.c (helper-things) */
typedef struct {
    int channels;
    int skip;
    int sample_rate;
    /* multichannel-only */
    int coupled_count;
    int stream_count;
    uint8_t channel_mapping[255];
    /* frame table */
    off_t table_offset;
    int table_count;
    /* fixed frames */
    uint16_t frame_size;
} opus_config;

ffmpeg_codec_data* init_ffmpeg_switch_opus_config(STREAMFILE* sf, off_t start_offset, size_t data_size, opus_config* cfg);
ffmpeg_codec_data* init_ffmpeg_switch_opus(STREAMFILE* sf, off_t start_offset, size_t data_size, int channels, int skip, int sample_rate);
ffmpeg_codec_data* init_ffmpeg_ue4_opus(STREAMFILE* sf, off_t start_offset, size_t data_size, int channels, int skip, int sample_rate);
ffmpeg_codec_data* init_ffmpeg_ea_opus(STREAMFILE* sf, off_t start_offset, size_t data_size, int channels, int skip, int sample_rate);
ffmpeg_codec_data* init_ffmpeg_ea_opusm(STREAMFILE* sf, off_t data_offset, size_t data_size, opus_config* cfg);
ffmpeg_codec_data* init_ffmpeg_x_opus(STREAMFILE* sf, off_t table_offset, int table_count, off_t data_offset, size_t data_size, int channels, int skip);
ffmpeg_codec_data* init_ffmpeg_fsb_opus(STREAMFILE* sf, off_t start_offset, size_t data_size, int channels, int skip, int sample_rate);
ffmpeg_codec_data* init_ffmpeg_wwise_opus(STREAMFILE* sf, off_t data_offset, size_t data_size, opus_config* cfg);
ffmpeg_codec_data* init_ffmpeg_fixed_opus(STREAMFILE* sf, off_t data_offset, size_t data_size, opus_config* cfg);

size_t switch_opus_get_samples(off_t offset, size_t stream_size, STREAMFILE* sf);

size_t switch_opus_get_encoder_delay(off_t offset, STREAMFILE* sf);
size_t ue4_opus_get_encoder_delay(off_t offset, STREAMFILE* sf);
size_t ea_opus_get_encoder_delay(off_t offset, STREAMFILE* sf);
size_t fsb_opus_get_encoder_delay(off_t offset, STREAMFILE* sf);

/* ffmpeg_decoder_custom_mp4.c*/
typedef struct {
    int channels;
    int sample_rate;
    int32_t num_samples;

    uint32_t stream_offset;
    uint32_t stream_size;
    uint32_t table_offset;
    uint32_t table_entries;

    int encoder_delay;
    int end_padding;
    int frame_samples;
} mp4_custom_t;

ffmpeg_codec_data* init_ffmpeg_mp4_custom_std(STREAMFILE* sf, mp4_custom_t* mp4);
ffmpeg_codec_data* init_ffmpeg_mp4_custom_lyn(STREAMFILE* sf, mp4_custom_t* mp4);

#endif

#endif /*_CODING_H*/
