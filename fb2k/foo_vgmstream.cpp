/**
 * vgmstream for foobar2000
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <io.h>
#include <locale.h>

#include <foobar2000/SDK/foobar2000.h>

extern "C" {
#include "../src/vgmstream.h"
#include "../src/vgmstream_types.h"
#include "../src/api.h"
#include "../src/coding/coding.h"
}
#include "foo_vgmstream.h"
#include "foo_filetypes.h"


#include "../version.h"
#ifndef VGMSTREAM_VERSION
#define VGMSTREAM_VERSION "unknown version " __DATE__
#endif

#define PLUGIN_NAME  "sli plugin"
#define PLUGIN_VERSION  VGMSTREAM_VERSION
#define PLUGIN_INFO  PLUGIN_NAME " " PLUGIN_VERSION " (" __DATE__ ")"
#define PLUGIN_DESCRIPTION  PLUGIN_INFO "\n" \
            "by lltcggie\n" \
            "\n" \
            "https://github.com/lltcggie/foo_input_sli\n"

#define PLUGIN_FILENAME "foo_input_sli.dll"


static void log_callback(int level, const char* str) {
    console::formatter() /*<< "vgmstream: "*/ << str;
}

#ifdef VGM_USE_FFMPEG
static bool set_ffmpeg_meta_data(file_info& p_info, ffmpeg_codec_data* data, const char* key) {
    const char* meta = ffmpeg_get_metadata_value(data, key);
    if (meta) {
        p_info.meta_set(key, meta);
        return true;
    }

    return false;
}

static bool set_ffmpeg_meta_data(file_info& p_info, ffmpeg_codec_data* data, const char* key_foobar2000, const char* key_ffmpeg) {
    const char* meta = ffmpeg_get_metadata_value(data, key_ffmpeg);
    if (meta) {
        p_info.meta_set(key_foobar2000, meta);
        return true;
    }

    return false;
}
#endif

// called every time a file is added to the playlist (to get info) or when playing
input_vgmstream::input_vgmstream() {
    vgmstream = NULL;
    output_channels = 0;

    decoding = false;
    paused = 0;
    decode_pos_ms = 0;
    decode_pos_samples = 0;
    length_samples = 0;

    fade_seconds = 10.0;
    fade_delay_seconds = 0.0;
    loop_count = 2.0;
    loop_forever = false;
    ignore_loop = 0;
    downmix_channels = 0;

    load_settings();

    vgmstream_set_log_callback(VGM_LOG_LEVEL_ALL, &log_callback);
}

// called on stop or when playlist info has been read
input_vgmstream::~input_vgmstream() {
    close_vgmstream(vgmstream);
    vgmstream = NULL;
}

// called first when a new file is accepted, before playing it
void input_vgmstream::open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort) {

    if (!p_path) { // shouldn't be possible
        throw exception_io_data();
        return; //???
    }

    filename = p_path;

    // allow non-existing files in some cases
    bool infile_virtual = !filesystem::g_exists(p_path, p_abort)
        && vgmstream_is_virtual_filename(filename) == 1;

    // don't try to open virtual files as it'll fail
    // (doesn't seem to have any adverse effect, except maybe no stats)
    // setup_vgmstream also makes further checks before file is finally opened
    if (!infile_virtual) {
        // keep file stats around (timestamp, filesize)
        if ( p_filehint.is_empty() )
            input_open_file_helper( p_filehint, filename, p_reason, p_abort );
        stats = p_filehint->get_stats( p_abort );

        uint32_t flags = stats2_legacy; //foobar2000_io.stats2_xxx, not sure about the implications
        stats2 = p_filehint->get_stats2_(flags, p_abort); // ???
    }

    switch(p_reason) {
        case input_open_decode: // prepare to retrieve info and decode
        case input_open_info_read: // prepare to retrieve info
            setup_vgmstream(p_abort); // must init vgmstream to get subsongs
            break;

        case input_open_info_write: // prepare to retrieve info and tag
            throw exception_io_data();
            break;

        default: // nothing else should be possible
            throw exception_io_data();
            break;
    }
}

// called before playing to get info
void input_vgmstream::get_info(file_info & p_info, abort_callback & p_abort) {

    int length_in_ms=0, channels = 0, samplerate = 0;
    int total_samples = -1;
    int bitrate = 0;
    int loop_flag = -1, loop_start = -1, loop_end = -1;
    pfc::string8 description;
    pfc::string8_fast temp;

    get_song_info(&length_in_ms, &total_samples, &loop_flag, &loop_start, &loop_end, &samplerate, &channels, &bitrate, description, p_info, p_abort);


    /* set tag info (metadata tab in file properties) */

    if (get_description_tag(temp,description,"stream count: ")) p_info.meta_set("stream_count",temp);
    if (get_description_tag(temp,description,"stream index: ")) p_info.meta_set("stream_index",temp);
    if (get_description_tag(temp,description,"stream name: ")) p_info.meta_set("stream_name",temp);
    if (loop_end) {
        p_info.meta_set("loop_start", pfc::format_int(loop_start));
        p_info.meta_set("loop_end", pfc::format_int(loop_end));
        // has extra text info
        //if (get_description_tag(temp,description,"loop start: ")) p_info.meta_set("loop_start",temp);
        //if (get_description_tag(temp,description,"loop end: ")) p_info.meta_set("loop_end",temp); 
    }


    /* set technical info (details tab in file properties) */

    p_info.info_set("vgmstream_version", PLUGIN_VERSION);
    p_info.info_set_int("samplerate", samplerate);
    p_info.info_set_int("channels", channels);
    p_info.info_set_int("bitspersample", 16);
    /* not quite accurate but some people are confused by "lossless"
     * (could set lossless if PCM, but then again PCMFloat or PCM8 are converted/"lossy" in vgmstream) */
    p_info.info_set("encoding","lossy/lossless");
    p_info.info_set_bitrate(bitrate / 1000);
    if (total_samples > 0)
        p_info.info_set_int("stream_total_samples", total_samples);
    if (loop_start >= 0 && loop_end > loop_start) {
        if (!loop_flag) p_info.info_set("looping", "disabled");
        p_info.info_set_int("loop_start", loop_start);
        p_info.info_set_int("loop_end", loop_end);
    }
    p_info.set_length(((double)length_in_ms)/1000);

    if (get_description_tag(temp,description,"encoding: ")) p_info.info_set("codec",temp);
    if (get_description_tag(temp,description,"layout: ")) p_info.info_set("layout",temp);
    if (get_description_tag(temp,description,"interleave: ",' ')) p_info.info_set("interleave",temp);
    if (get_description_tag(temp,description,"interleave last block:",' ')) p_info.info_set("interleave_last_block",temp);

    if (get_description_tag(temp,description,"block size: ")) p_info.info_set("block_size",temp);
    if (get_description_tag(temp,description,"metadata from: ")) p_info.info_set("metadata_source",temp);
    if (get_description_tag(temp,description,"stream count: ")) p_info.info_set("stream_count",temp);
    if (get_description_tag(temp,description,"stream index: ")) p_info.info_set("stream_index",temp);
    if (get_description_tag(temp,description,"stream name: ")) p_info.info_set("stream_name",temp);

    if (get_description_tag(temp,description,"channel mask: ")) p_info.info_set("channel_mask",temp);
    if (get_description_tag(temp,description,"output channels: ")) p_info.info_set("output_channels",temp);
    if (get_description_tag(temp,description,"input channels: ")) p_info.info_set("input_channels",temp);

}

t_filestats input_vgmstream::get_file_stats(abort_callback & p_abort) {
    return stats;
}

t_filestats2 input_vgmstream::get_stats2(uint32_t f, abort_callback & p_abort) {
    return stats2;
}

// called right before actually playing (decoding) a song/subsong
//void input_vgmstream::decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort) {
void input_vgmstream::decode_initialize(unsigned p_flags, abort_callback& p_abort) {

    // "don't loop forever" flag (set when converting to file, scanning for replaygain, etc)
    // flag is set *after* loading vgmstream + applying config so manually disable
    bool force_ignore_loop = !!(p_flags & input_flag_no_looping);
    if (force_ignore_loop) // could always set but vgmstream is re-created on play start
        vgmstream_set_play_forever(vgmstream, 0);

    decode_seek(0, p_abort);
};

// called when audio buffer needs to be filled
bool input_vgmstream::decode_run(audio_chunk & p_chunk, abort_callback & p_abort) {
    if (!decoding) return false;
    if (!vgmstream) return false;

    int max_buffer_samples = SAMPLE_BUFFER_SIZE;
    int samples_to_do = max_buffer_samples;
    t_size bytes;

    {
        bool play_forever = vgmstream_get_play_forever(vgmstream);
        if (decode_pos_samples + max_buffer_samples > length_samples && !play_forever)
            samples_to_do = length_samples - decode_pos_samples;
        else
            samples_to_do = max_buffer_samples;

        if (samples_to_do == 0) { /*< DECODE_SIZE*/
            decoding = false;
            return false; /* EOF, didn't decode samples in this call */
        }

        render_vgmstream(sample_buffer, samples_to_do, vgmstream);

        unsigned channel_config = vgmstream->channel_layout;
        if (!channel_config)
            channel_config = audio_chunk::g_guess_channel_config(output_channels);

        bytes = (samples_to_do * output_channels * sizeof(sample_buffer[0]));
        p_chunk.set_data_fixedpoint((char*)sample_buffer, bytes, vgmstream->sample_rate, output_channels, 16, channel_config);

        decode_pos_samples += samples_to_do;
        decode_pos_ms = decode_pos_samples * 1000LL / vgmstream->sample_rate;

        return true; /* decoded in this call (sample_buffer or less) */
    }
}

// called when seeking
void input_vgmstream::decode_seek(double p_seconds, abort_callback & p_abort) {
    int32_t seek_sample = (int32_t)audio_math::time_to_samples(p_seconds, vgmstream->sample_rate);
    bool play_forever = vgmstream_get_play_forever(vgmstream);

    // possible when disabling looping without refreshing foobar's cached song length
    // (p_seconds can't go over seek bar with infinite looping on, though)
    if (seek_sample > length_samples)
        seek_sample = length_samples;

    seek_vgmstream(vgmstream, seek_sample);

    decode_pos_samples = seek_sample;
    decode_pos_ms = decode_pos_samples * 1000LL / vgmstream->sample_rate;
    decoding = play_forever || decode_pos_samples < length_samples;
}

bool input_vgmstream::decode_can_seek() { return true; }
bool input_vgmstream::decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { return false; }
bool input_vgmstream::decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { return false; }
void input_vgmstream::decode_on_idle(abort_callback & p_abort) { /*m_file->on_idle(p_abort);*/ }

bool input_vgmstream::g_is_our_content_type(const char * p_content_type) { return false; }

// called to check if file can be processed by the plugin
bool input_vgmstream::g_is_our_path(const char * p_path, const char * p_extension) {
    vgmstream_ctx_valid_cfg cfg = {0};

    cfg.is_extension = 1;
    return vgmstream_ctx_is_valid(p_extension, &cfg) > 0 ? true : false;
}

// internal util to create a VGMSTREAM
VGMSTREAM* input_vgmstream::init_vgmstream_foo(const char * const filename, abort_callback & p_abort) {
    VGMSTREAM* vgmstream = NULL;

    /* Workaround for a foobar bug (mainly for complex TXTP):
     * When converting to .ogg foobar calls oggenc, that calls setlocale(LC_ALL, "") to use system's locale.
     * After that, text parsing using sscanf that expects US locale for "N.N" decimals fails in some locales,
     * so reset it here just in case
     * (maybe should be done on lib and/or restore original locale but it's not common to change it in C) */
    //const char* original_locale = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, "C");

    STREAMFILE* sf = open_foo_streamfile(filename, &p_abort, &stats);
    if (sf) {
        vgmstream = init_vgmstream_from_STREAMFILE(sf);
        close_streamfile(sf);
    }

    //setlocale(LC_ALL, original_locale);
    return vgmstream;
}

// internal util to initialize vgmstream
void input_vgmstream::setup_vgmstream(abort_callback & p_abort) {
    // close first in case of changing subsongs
    if (vgmstream) {
        close_vgmstream(vgmstream);
    }

    // subsong and filename are always defined before this
    vgmstream = init_vgmstream_foo(filename, p_abort);
    if (!vgmstream) {
        throw exception_io_data();
        return;
    }

    apply_config(vgmstream);

    /* enable after all config but before outbuf (though ATM outbuf is not dynamic so no need to read input_channels) */
    vgmstream_mixing_autodownmix(vgmstream, downmix_channels);
    vgmstream_mixing_enable(vgmstream, SAMPLE_BUFFER_SIZE, NULL /*&input_channels*/, &output_channels);

    decode_pos_ms = 0;
    decode_pos_samples = 0;
    paused = 0;
    length_samples = vgmstream_get_samples(vgmstream);
}

// internal util to get info
void input_vgmstream::get_song_info(int *length_in_ms, int *total_samples, int *loop_flag, int *loop_start, int *loop_end, int *sample_rate, int *channels, int *bitrate, pfc::string_base & description, file_info& p_info, abort_callback & p_abort) {
    VGMSTREAM* infostream = NULL;
    bool is_infostream = false;
    char temp[1024];
	int info_channels;

	infostream = vgmstream;
	info_channels = output_channels;

	if (length_in_ms) {
		*length_in_ms = -1000;
        if (infostream) {
            *channels = info_channels;
            *sample_rate = infostream->sample_rate;
            *total_samples = infostream->num_samples;
            *bitrate = get_vgmstream_average_bitrate(infostream);
            *loop_flag = infostream->loop_flag;
            *loop_start = infostream->loop_start_sample;
            *loop_end = infostream->loop_end_sample;

            int num_samples = vgmstream_get_samples(infostream);
            *length_in_ms = num_samples*1000LL / infostream->sample_rate;

            describe_vgmstream(infostream, temp, sizeof(temp));
            description = temp;
        }
    }

#ifdef VGM_USE_FFMPEG
    if (infostream && infostream->coding_type == coding_FFmpeg) {
        ffmpeg_codec_data* data = (ffmpeg_codec_data * )vgmstream->codec_data;

        replaygain_info rp_info;
        rp_info.set_album_gain_text(ffmpeg_get_metadata_value(data, "replaygain_album_gain"));
        rp_info.set_track_gain_text(ffmpeg_get_metadata_value(data, "replaygain_track_gain"));
        rp_info.set_album_peak_text(ffmpeg_get_metadata_value(data, "replaygain_album_peak"));
        rp_info.set_track_peak_text(ffmpeg_get_metadata_value(data, "replaygain_track_peak"));
        p_info.set_replaygain(rp_info);

        set_ffmpeg_meta_data(p_info, data, "ARTIST");
        set_ffmpeg_meta_data(p_info, data, "TITLE");
        set_ffmpeg_meta_data(p_info, data, "ALBUM");
        set_ffmpeg_meta_data(p_info, data, "DATE");
        set_ffmpeg_meta_data(p_info, data, "GENRE");
        set_ffmpeg_meta_data(p_info, data, "COMPOSER");
        set_ffmpeg_meta_data(p_info, data, "PERFORMER");
        set_ffmpeg_meta_data(p_info, data, "ALBUM ARTIST", "album_artist");
        set_ffmpeg_meta_data(p_info, data, "TRACKNUMBER", "track");
        set_ffmpeg_meta_data(p_info, data, "TOTALTRACKS");
        set_ffmpeg_meta_data(p_info, data, "DISCNUMBER", "disc");
        set_ffmpeg_meta_data(p_info, data, "TOTALDISCS");
        set_ffmpeg_meta_data(p_info, data, "COMMENT");
        set_ffmpeg_meta_data(p_info, data, "ALBUMSORT");
    }
#endif

    // and only close if was querying a new subsong
    if (is_infostream) {
        close_vgmstream(infostream);
        infostream = NULL;
    }
}

bool input_vgmstream::get_description_tag(pfc::string_base & temp, pfc::string_base const& description, const char *tag, char delimiter) {
    // extract a "tag" from the description string
    t_size pos = description.find_first(tag);
    t_size eos;
    if (pos != pfc::infinite_size) {
        pos += strlen(tag);
        eos = description.find_first(delimiter, pos);
        if (eos == pfc::infinite_size) eos = description.length();
        temp.set_string(description + pos, eos - pos);
        //console::formatter() << "tag=" << tag << ", delim=" << delimiter << "temp=" << temp << ", pos=" << pos << "" << eos;
        return true;
    }
    return false;
}

void input_vgmstream::apply_config(VGMSTREAM* vgmstream) {
    vgmstream_cfg_t vcfg = {0};

    vcfg.allow_play_forever = 1;
    vcfg.play_forever = loop_forever;
    vcfg.loop_count = loop_count;
    vcfg.fade_time = fade_seconds;
    vcfg.fade_delay = fade_delay_seconds;
    vcfg.ignore_loop = ignore_loop;

    vgmstream_apply_config(vgmstream, &vcfg);
}

GUID input_vgmstream::g_get_guid() {
    static const GUID guid = { 0xf29117f, 0x4f82, 0x4514, { 0x84, 0x77, 0x8e, 0x4f, 0x38, 0x3d, 0xe3, 0x58 } };
    return guid;
}

const char * input_vgmstream::g_get_name() {
    return "sli";
}

GUID input_vgmstream::g_get_preferences_guid() {
    static const GUID guid = { 0xc208efaf, 0x68da, 0x4938, { 0x9d, 0xb4, 0x81, 0xcc, 0x14, 0x60, 0x59, 0x68 } };
    return guid;
}

// checks priority (foobar 1.4+)
bool input_vgmstream::g_is_low_merit() {
    return true;
}

// foobar plugin defs
static input_singletrack_factory_t<input_vgmstream> g_input_vgmstream_factory;

DECLARE_COMPONENT_VERSION(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_DESCRIPTION);
VALIDATE_COMPONENT_FILENAME(PLUGIN_FILENAME);
DECLARE_FILE_TYPE("SLI files", "*.SLI");
