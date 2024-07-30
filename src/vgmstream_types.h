#ifndef _VGMSTREAM_TYPES_H
#define _VGMSTREAM_TYPES_H


/* The encoding type specifies the format the sound data itself takes */
typedef enum {
    coding_SILENCE,         /* generates silence */

#ifdef VGM_USE_FFMPEG
    coding_FFmpeg,          /* Formats handled by FFmpeg (ATRAC3, XMA, AC3, etc) */
#endif
} coding_t;

/* The layout type specifies how the sound data is laid out in the file */
typedef enum {
    /* generic */
    layout_none,            /* straight data */

} layout_t;

/* The meta type specifies how we know what we know about the file. */
typedef enum {
    meta_SILENCE,

    meta_FFMPEG,
    meta_FFMPEG_faulty,

} meta_t;

#endif
