#include "vgmstream_init.h"

//typedef VGMSTREAM* (*init_vgmstream_t)(STREAMFILE*);

/* list of metadata parser functions that will recognize files, used on init */
init_vgmstream_t init_vgmstream_functions[] = {
    init_vgmstream_sli_loops,

#ifdef VGM_USE_FFMPEG
    init_vgmstream_ffmpeg,          /* may play anything incorrectly, since FFmpeg doesn't check extensions */
#endif
};

#define LOCAL_ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))
static const int init_vgmstream_count = LOCAL_ARRAY_LENGTH(init_vgmstream_functions);


VGMSTREAM* detect_vgmstream_format(STREAMFILE* sf) {
    if (!sf)
        return NULL;

    /* try a series of formats, see which works */
    for (int i = 0; i < init_vgmstream_count; i++) {
        init_vgmstream_t init_vgmstream_function = init_vgmstream_functions[i];
    
        /* call init function and see if valid VGMSTREAM was returned */
        VGMSTREAM* vgmstream = init_vgmstream_function(sf);
        if (!vgmstream)
            continue;

        vgmstream->format_id = i + 1;

        /* validate + setup vgmstream */
        if (!prepare_vgmstream(vgmstream, sf)) {
            /* keep trying if wasn't valid, as simpler formats may return a vgmstream by mistake */
            close_vgmstream(vgmstream);
            continue;
        }

        return vgmstream;
    }

    /* not supported */
    return NULL;
}

init_vgmstream_t get_vgmstream_format_init(int format_id) {
    // ID is expected to be from 1...N, to distinguish from 0 = not set
    if (format_id <= 0 || format_id > init_vgmstream_count)
        return NULL;

    return init_vgmstream_functions[format_id - 1];
}
