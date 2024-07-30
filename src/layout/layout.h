#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "../streamtypes.h"
#include "../vgmstream.h"
#include "../util/reader_sf.h"
#include "../util/log.h"

/* other layouts */
void render_vgmstream_flat(sample_t* buffer, int32_t sample_count, VGMSTREAM* vgmstream);


#endif
