#include "vgmstream.h"
#include "coding/coding.h"


/* Defines the list of accepted extensions. vgmstream doesn't use it internally so it's here
 * to inform plugins that need it. Common extensions are commented out to avoid stealing them
 * and possibly adding an unwanted association to the player. */

/* Common extensions (like .wav or .ogg) should go in the common_extension_list. It should only
 * contain common formats that vgmstream can also parse, to avoid hijacking them (since their
 * plugins typically are faster and have desirable features vgmstream won't handle). Extensions of
 * formats not parsed don't need to go there (for example .stm is a Scream Tracker Module elsewhere,
 * but our .stm is very different so there is no conflict). */

/* Some extensions require external libraries and could be #ifdef, not worth. */

/* Formats marked as "not parsed" mean they'll go through FFmpeg, the header/extension isn't
 * parsed by vgmstream and typically won't not be fully accurate. */


static const char* extension_list[] = {
    //"", /* vgmstream can play extensionless files too, but plugins must accept them manually */

    "sli",

    //, NULL //end mark
};

static const char* common_extension_list[] = {
    "ali", //common
};


/* List supported formats and return elements in the list, for plugins that need to know. */
const char** vgmstream_get_formats(size_t* size) {
    if (!size)
        return NULL;

    *size = sizeof(extension_list) / sizeof(char*);
    return extension_list;
}

const char** vgmstream_get_common_formats(size_t* size) {
    if (!size)
        return NULL;

    *size = sizeof(common_extension_list) / sizeof(char*);
    return common_extension_list;
}


/* internal description info */

typedef struct {
    coding_t type;
    const char *description;
} coding_info;

typedef struct {
    layout_t type;
    const char *description;
} layout_info;

typedef struct {
    meta_t type;
    const char *description;
} meta_info;


static const coding_info coding_info_list[] = {
        {coding_SILENCE,            "Silence"},

#ifdef VGM_USE_FFMPEG
        {coding_FFmpeg,             "FFmpeg"},
#endif
};

static const layout_info layout_info_list[] = {
        {layout_none,                   "flat"},
};

static const meta_info meta_info_list[] = {
        {meta_SILENCE,              "Silence"},
        {meta_FFMPEG,               "FFmpeg supported format"},
        {meta_FFMPEG_faulty,        "FFmpeg supported format (check log)"},
};

void get_vgmstream_coding_description(VGMSTREAM* vgmstream, char* out, size_t out_size) {
    int i, list_length;
    const char *description;

    description = "CANNOT DECODE";

    switch (vgmstream->coding_type) {
#ifdef VGM_USE_FFMPEG
        case coding_FFmpeg:
            description = ffmpeg_get_codec_name(vgmstream->codec_data);
            if (description == NULL)
                description = "FFmpeg";
            break;
#endif
        default:
            list_length = sizeof(coding_info_list) / sizeof(coding_info);
            for (i = 0; i < list_length; i++) {
                if (coding_info_list[i].type == vgmstream->coding_type)
                    description = coding_info_list[i].description;
            }
            break;
    }

    strncpy(out, description, out_size);
}

static const char* get_layout_name(layout_t layout_type) {
    int i, list_length;

    list_length = sizeof(layout_info_list) / sizeof(layout_info);
    for (i = 0; i < list_length; i++) {
        if (layout_info_list[i].type == layout_type)
            return layout_info_list[i].description;
    }

    return NULL;
}

/* Makes a mixed description, considering a segments/layers can contain segments/layers infinitely, like:
 *
 * "(L3[S2L2]S3)"        "(S3[L2[S2S2]])"
 *  L3                    S3
 *    S2                    L2
 *      file                  S2
 *      file                    file
 *    file                      file
 *    L2                      file
 *      file                file
 *      file                file
 *
 * ("mixed" is added externally)
 */
static int get_layout_mixed_description(VGMSTREAM* vgmstream, char* dst, int dst_size) {
    int i, count, done = 0;
    VGMSTREAM** vgmstreams = NULL;

    if (!vgmstreams || done == 0 || done >= dst_size)
        return 0;

    if (done + 1 < dst_size) {
        dst[done++] = '[';
    }

    for (i = 0; i < count; i++) {
        done += get_layout_mixed_description(vgmstreams[i], dst + done, dst_size - done);
    }

    if (done + 1 < dst_size) {
        dst[done++] = ']';
    }

    return done;
}

void get_vgmstream_layout_description(VGMSTREAM* vgmstream, char* out, size_t out_size) {
    const char* description;
    int mixed = 0;

    description = get_layout_name(vgmstream->layout_type);
    if (!description) description = "INCONCEIVABLE";

    snprintf(out, out_size, "%s", description);

    if (mixed) {
        char tmp[256] = {0};

        get_layout_mixed_description(vgmstream, tmp, sizeof(tmp) - 1);
        snprintf(out, out_size, "mixed (%s)", tmp);
        return;
    }
}

void get_vgmstream_meta_description(VGMSTREAM* vgmstream, char* out, size_t out_size) {
    int i, list_length;
    const char* description;

    description = "THEY SHOULD HAVE SENT A POET";

    list_length = sizeof(meta_info_list) / sizeof(meta_info);
    for (i=0; i < list_length; i++) {
        if (meta_info_list[i].type == vgmstream->meta_type)
            description = meta_info_list[i].description;
    }

    strncpy(out, description, out_size);
}
