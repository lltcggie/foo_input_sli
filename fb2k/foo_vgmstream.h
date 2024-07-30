#ifndef _FOO_VGMSTREAM_
#define _FOO_VGMSTREAM_

#define SAMPLE_BUFFER_SIZE     1024

extern "C" {
#include "../src/vgmstream.h"
}


class input_vgmstream : public input_stubs {
    public:
        input_vgmstream();
        ~input_vgmstream();

        void open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort);

        void get_info(file_info& p_info, abort_callback& p_abort);
        t_filestats get_file_stats(abort_callback & p_abort);
        t_filestats2 get_stats2(uint32_t f, abort_callback & p_abort);

        void decode_initialize(unsigned p_flags, abort_callback& p_abort);
        bool decode_run(audio_chunk & p_chunk, abort_callback & p_abort);
        void decode_seek(double p_seconds, abort_callback & p_abort);
        bool decode_can_seek();
        bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
        bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
        void decode_on_idle(abort_callback & p_abort);

        void retag(const file_info& p_info, abort_callback& p_abort) { throw exception_tagging_unsupported(); }
        void remove_tags(abort_callback&) { throw exception_tagging_unsupported(); }

        static bool g_is_our_content_type(const char * p_content_type);
        static bool g_is_our_path(const char * p_path, const char * p_extension);

        static GUID g_get_guid();
        static const char * g_get_name();
        static GUID g_get_preferences_guid();
        static bool g_is_low_merit();

    private:
        //service_ptr_t<file> m_file;
        pfc::string8 filename;
        t_filestats stats;
        t_filestats2 stats2;

        /* state */
        VGMSTREAM* vgmstream;
        int output_channels;

        bool decoding;
        int paused;
        int decode_pos_ms;
        int decode_pos_samples;
        int length_samples;
        short sample_buffer[SAMPLE_BUFFER_SIZE * VGMSTREAM_MAX_CHANNELS];

        /* settings */
        double fade_seconds;
        double fade_delay_seconds;
        double loop_count;
        bool loop_forever;
        int ignore_loop;

        int downmix_channels;

        /* helpers */
        VGMSTREAM* init_vgmstream_foo(const char* const filename, abort_callback& p_abort);
        void setup_vgmstream(abort_callback& p_abort);
        void load_settings();
        void get_song_info(int* length_in_ms, int* total_samples, int* loop_flag, int *loop_start, int* loop_end, int* sample_rate, int* channels, int* bitrate, pfc::string_base& description, file_info& p_info, abort_callback& p_abort);
        bool get_description_tag(pfc::string_base& temp, pfc::string_base const& description, const char* tag, char delimiter = '\n');
        void apply_config(VGMSTREAM* vgmstream);
};

/* foo_streamfile.cpp */
STREAMFILE* open_foo_streamfile(const char* const filename, abort_callback* p_abort, t_filestats* stats);


#endif /*_FOO_VGMSTREAM_*/
