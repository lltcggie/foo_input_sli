#ifndef _META_H
#define _META_H

#include "../vgmstream.h"
#include "../util/reader_sf.h"
#include "../util/reader_text.h"
#include "../util/sf_utils.h"
#include "../util/log.h"

typedef VGMSTREAM* (*init_vgmstream_t)(STREAMFILE* sf);

VGMSTREAM* init_vgmstream_silence(int channels, int sample_rate, int32_t num_samples);
VGMSTREAM* init_vgmstream_silence_container(int total_subsongs);
VGMSTREAM* init_vgmstream_silence_base(VGMSTREAM* vgmstream);


#ifdef VGM_USE_FFMPEG
VGMSTREAM* init_vgmstream_ffmpeg(STREAMFILE* sf);
#endif

VGMSTREAM* init_vgmstream_sli_loops(STREAMFILE* sf);

#endif /*_META_H*/
