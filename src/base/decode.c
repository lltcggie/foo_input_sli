#include "../vgmstream.h"
#include "../layout/layout.h"
#include "../coding/coding.h"
#include "decode.h"
#include "mixing.h"
#include "plugins.h"

/* custom codec handling, not exactly "decode" stuff but here to simplify adding new codecs */


void decode_free(VGMSTREAM* vgmstream) {
    if (!vgmstream->codec_data)
        return;

#ifdef VGM_USE_FFMPEG
    if (vgmstream->coding_type == coding_FFmpeg) {
        free_ffmpeg(vgmstream->codec_data);
    }
#endif
}


void decode_seek(VGMSTREAM* vgmstream) {
    if (!vgmstream->codec_data)
        return;

#ifdef VGM_USE_FFMPEG
    if (vgmstream->coding_type == coding_FFmpeg) {
        seek_ffmpeg(vgmstream->codec_data, vgmstream->loop_current_sample);
    }
#endif
}


void decode_reset(VGMSTREAM* vgmstream) {
    if (!vgmstream->codec_data)
        return;

#ifdef VGM_USE_FFMPEG
    if (vgmstream->coding_type == coding_FFmpeg) {
        reset_ffmpeg(vgmstream->codec_data);
    }
#endif
}


/* Get the number of samples of a single frame (smallest self-contained sample group, 1/N channels) */
int decode_get_samples_per_frame(VGMSTREAM* vgmstream) {
    /* Value returned here is the max (or less) that vgmstream will ask a decoder per
     * "decode_x" call. Decoders with variable samples per frame or internal discard
     * may return 0 here and handle arbitrary samples_to_do values internally
     * (or some internal sample buffer max too). */

    switch (vgmstream->coding_type) {
        case coding_SILENCE:
            return 0;

#ifdef VGM_USE_FFMPEG
        case coding_FFmpeg:
            return 0;
#endif
        default:
            return 0;
    }
}

/* Get the number of bytes of a single frame (smallest self-contained byte group, 1/N channels) */
int decode_get_frame_size(VGMSTREAM* vgmstream) {
    switch (vgmstream->coding_type) {
        case coding_SILENCE:
            return 0;

#ifdef VGM_USE_FFMPEG
        case coding_FFmpeg:
            return vgmstream->interleave_block_size;
#endif
        /* UBI_ADPCM: varies per mode? */
        /* IMUSE: VBR */
        /* EA_MT: VBR, frames of bit counts or PCM frames */
        /* COMPRESSWAVE: VBR/huffman bits */
        /* ATRAC9: CBR around  0x100-200 */
        /* CELT FSB: varies, usually 0x80-100 */
        /* SPEEX: varies, usually 0x40-60 */
        /* TAC: VBR around ~0x200-300 */
        /* Vorbis, MPEG, ACM, etc: varies */
        default: /* (VBR or managed by decoder) */
            return 0;
    }
}

/* In NDS IMA the frame size is the block size, so the last one is short */
int decode_get_samples_per_shortframe(VGMSTREAM* vgmstream) {
    switch (vgmstream->coding_type) {
        default:
            return decode_get_samples_per_frame(vgmstream);
    }
}

int decode_get_shortframe_size(VGMSTREAM* vgmstream) {
    switch (vgmstream->coding_type) {
        default:
            return decode_get_frame_size(vgmstream);
    }
}

/* ugly kludge due to vgmstream's finicky internals, to be improved some day:
 * - some codecs have frame sizes AND may also have interleave
 * - meaning, ch0 could read 0x100 (frame_size) N times until 0x1000 (interleave)
 *   then skip 0x1000 per other channels and keep reading 0x100
 *   (basically: ch0=0x0000..0x1000, ch1=0x1000..0x2000, ch0=0x2000..0x3000, etc)
 * - interleave layout assumes by default codecs DON'T update offsets and only interleave does
 *   - interleave calculates how many frames/samples will read before moving offsets,
 *     then once 1 channel is done skips original channel data + other channel's data
 *   - decoders need to calculate current frame offset on every frame since
 *     offsets only move when interleave moves offsets (ugly)
 * - other codecs move offsets internally instead (also ugly)
 *   - but interleave doesn't know this and will skip too much data
 * 
 * To handle the last case, return a flag here that interleave layout can use to
 * separate between both cases when the interleave data is done 
 * - codec doesn't advance offsets: will skip interleave for all channels including current
 *   - ex. 2ch, 0x100, 0x1000: after reading 0x100*10 frames offset is still 0x0000 > skips 0x1000*2 (ch0+ch1)
 * - codec does advance offsets: will skip interleave for all channels except current
 *   - ex. 2ch, 0x100, 0x1000: after reading 0x100*10 frames offset is at 0x1000 >  skips 0x1000*1 (ch1)
 * 
 * Ideally frame reading + skipping would be moved to some kind of consumer functions
 * separate from frame decoding which would simplify all this but meanwhile...
 * 
 * Instead of this flag, codecs could be converted to avoid moving offsets (like most codecs) but it's
 * getting hard to understand the root issue so have some wall of text as a reminder.
 */
bool decode_uses_internal_offset_updates(VGMSTREAM* vgmstream) {
    return false;
}

/* Decode samples into the buffer. Assume that we have written samples_written into the
 * buffer already, and we have samples_to_do consecutive samples ahead of us (won't call
 * more than one frame if configured above to do so).
 * Called by layouts since they handle samples written/to_do */
void decode_vgmstream(VGMSTREAM* vgmstream, int samples_written, int samples_to_do, sample_t* buffer) {
    int ch;

    buffer += samples_written * vgmstream->channels; /* passed externally to simplify I guess */

    switch (vgmstream->coding_type) {
        case coding_SILENCE:
            memset(buffer, 0, samples_to_do * vgmstream->channels * sizeof(sample_t));
            break;

#ifdef VGM_USE_FFMPEG
        case coding_FFmpeg:
            decode_ffmpeg(vgmstream, buffer, samples_to_do, vgmstream->channels);
            break;
#endif
        default:
            break;
    }
}

/* Calculate number of consecutive samples we can decode. Takes into account hitting
 * a loop start or end, or going past a single frame. */
int decode_get_samples_to_do(int samples_this_block, int samples_per_frame, VGMSTREAM* vgmstream) {
    int samples_to_do;
    int samples_left_this_block;

    samples_left_this_block = samples_this_block - vgmstream->samples_into_block;
    samples_to_do = samples_left_this_block; /* by default decodes all samples left */

    /* fun loopy crap, why did I think this would be any simpler? */
    if (vgmstream->loop_flag) {
        int samples_after_decode = vgmstream->current_sample + samples_left_this_block;

        /* are we going to hit the loop end during this block? */
        if (samples_after_decode > vgmstream->loop_end_sample) {
            /* only do samples up to loop end */
            samples_to_do = vgmstream->loop_end_sample - vgmstream->current_sample;
        }

        /* are we going to hit the loop start during this block? (first time only) */
        if (samples_after_decode > vgmstream->loop_start_sample && !vgmstream->hit_loop) {
            /* only do samples up to loop start */
            samples_to_do = vgmstream->loop_start_sample - vgmstream->current_sample;
        }
    }

    /* if it's a framed encoding don't do more than one frame */
    if (samples_per_frame > 1 && (vgmstream->samples_into_block % samples_per_frame) + samples_to_do > samples_per_frame)
        samples_to_do = samples_per_frame - (vgmstream->samples_into_block % samples_per_frame);

    return samples_to_do;
}

/* Detect loop start and save values, or detect loop end and restore (loop back).
 * Returns 1 if loop was done. */
int decode_do_loop(VGMSTREAM* vgmstream) {
    /*if (!vgmstream->loop_flag) return 0;*/

    /* is this the loop end? = new loop, continue from loop_start_sample */
    if (vgmstream->current_sample == vgmstream->loop_end_sample) {

        /* disable looping if target count reached and continue normally
         * (only needed with the "play stream end after looping N times" option enabled) */
        vgmstream->loop_count++;
        if (vgmstream->loop_target && vgmstream->loop_target == vgmstream->loop_count) {
            vgmstream->loop_flag = 0; /* could be improved but works ok, will be restored on resets */
            return 0;
        }

        //TODO: improve
        /* loop codecs that need special handling, usually:
         * - on hit_loop, current offset is copied to loop_ch[].offset
         * - some codecs will overwrite loop_ch[].offset with a custom value
         * - loop_ch[] is copied to ch[] (with custom value)
         * - then codec will use ch[]'s offset
         * regular codecs may use copied loop_ch[] offset without issue */
        decode_seek(vgmstream);

        /* restore! */
        memcpy(vgmstream->ch, vgmstream->loop_ch, sizeof(VGMSTREAMCHANNEL) * vgmstream->channels);
        vgmstream->current_sample = vgmstream->loop_current_sample;
        vgmstream->samples_into_block = vgmstream->loop_samples_into_block;
        vgmstream->current_block_size = vgmstream->loop_block_size;
        vgmstream->current_block_samples = vgmstream->loop_block_samples;
        vgmstream->current_block_offset = vgmstream->loop_block_offset;
        vgmstream->next_block_offset = vgmstream->loop_next_block_offset;
        //vgmstream->pstate = vgmstream->lstate; /* play state is applied over loops */

        return 1; /* looped */
    }


    /* is this the loop start? save if we haven't saved yet (right when first loop starts) */
    if (!vgmstream->hit_loop && vgmstream->current_sample == vgmstream->loop_start_sample) {
        /* save! */
        memcpy(vgmstream->loop_ch, vgmstream->ch, sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);
        vgmstream->loop_current_sample = vgmstream->current_sample;
        vgmstream->loop_samples_into_block = vgmstream->samples_into_block;
        vgmstream->loop_block_size = vgmstream->current_block_size;
        vgmstream->loop_block_samples = vgmstream->current_block_samples;
        vgmstream->loop_block_offset = vgmstream->current_block_offset;
        vgmstream->loop_next_block_offset = vgmstream->next_block_offset;
        //vgmstream->lstate = vgmstream->pstate; /* play state is applied over loops */

        vgmstream->hit_loop = true; /* info that loop is now ready to use */
    }

    return 0; /* not looped */
}
