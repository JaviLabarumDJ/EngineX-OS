/* Unit tests for EngineX MIDI FX Daemon and DSP Integration (#180).
 *
 * Compiles and runs unprivileged:
 *   gcc -std=c11 -O2 -Wall -Wextra -Werror tools/enginex-midid/test_enginex_midid.c -lm -o /tmp/test_enginex_midid && /tmp/test_enginex_midid
 */

#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui-mod/midi_fx_shm.h"
#include "../ui-mod/pioneer_fx.h"

/* Forward declare processor from enginex_midid.c */
void enginex_midid_process_msg(struct sc_midi_fx_state *st, const uint8_t *msg, int verbose);

#include "enginex_midid.c"

static void test_initial_state(struct sc_midi_fx_state *st)
{
    assert(st->active_mode == SC_PFX_OFF);
    assert(st->wet_dry == 1.0f);
    assert(st->pad_pressed_mask == 0);
    assert(st->stems_level[0] == 1.0f);
    assert(st->stems_level[3] == 1.0f);
    printf("test_initial_state: PASS\n");
}

static void test_xpad_rolls(struct sc_midi_fx_state *st)
{
    /* Pad 1: Roll 1/8 beat (note 36, velocity 120) */
    uint8_t p1_on[3] = {0x90, 36, 120};
    enginex_midid_process_msg(st, p1_on, 0);
    assert(st->active_mode == SC_PFX_SLIP_ROLL);
    assert(st->roll_frames == 1500);
    assert(st->pad_pressed_mask == 0x01);

    /* Pad 2: Roll 1/4 beat while Pad 1 held (trill) */
    uint8_t p2_on[3] = {0x90, 37, 120};
    enginex_midid_process_msg(st, p2_on, 0);
    assert(st->active_mode == SC_PFX_SLIP_ROLL);
    assert(st->roll_frames == 3000);
    assert(st->pad_pressed_mask == 0x03);

    /* Release Pad 2: drops back to Pad 1 (1/8 beat) */
    uint8_t p2_off[3] = {0x80, 37, 0};
    enginex_midid_process_msg(st, p2_off, 0);
    assert(st->active_mode == SC_PFX_SLIP_ROLL);
    assert(st->roll_frames == 1500);
    assert(st->pad_pressed_mask == 0x01);

    /* Release Pad 1: all pads released, mode becomes OFF */
    uint8_t p1_off[3] = {0x80, 36, 0};
    enginex_midid_process_msg(st, p1_off, 0);
    assert(st->active_mode == SC_PFX_OFF);
    assert(st->pad_pressed_mask == 0x00);

    /* Pad 3: Roll 1/2 beat (note 38) */
    uint8_t p3_on[3] = {0x90, 38, 100};
    enginex_midid_process_msg(st, p3_on, 0);
    assert(st->active_mode == SC_PFX_SLIP_ROLL);
    assert(st->roll_frames == 6000);

    /* Release via Note-on with velocity 0 */
    uint8_t p3_off[3] = {0x90, 38, 0};
    enginex_midid_process_msg(st, p3_off, 0);
    assert(st->active_mode == SC_PFX_OFF);

    /* Pad 4: Roll 1/1 beat (note 39) */
    uint8_t p4_on[3] = {0x90, 39, 100};
    enginex_midid_process_msg(st, p4_on, 0);
    assert(st->active_mode == SC_PFX_SLIP_ROLL);
    assert(st->roll_frames == 12000);

    uint8_t p4_off[3] = {0x80, 39, 0};
    enginex_midid_process_msg(st, p4_off, 0);
    assert(st->active_mode == SC_PFX_OFF);

    printf("test_xpad_rolls: PASS\n");
}

static void test_scene_fx(struct sc_midi_fx_state *st)
{
    /* Pad 5: Spiral */
    uint8_t p5_on[3] = {0x90, 40, 100};
    enginex_midid_process_msg(st, p5_on, 0);
    assert(st->active_mode == SC_PFX_SPIRAL);
    uint8_t p5_off[3] = {0x80, 40, 0};
    enginex_midid_process_msg(st, p5_off, 0);
    assert(st->active_mode == SC_PFX_OFF);

    /* Pad 6: Dub Echo */
    uint8_t p6_on[3] = {0x90, 41, 100};
    enginex_midid_process_msg(st, p6_on, 0);
    assert(st->active_mode == SC_PFX_DUB_ECHO);
    uint8_t p6_off[3] = {0x80, 41, 0};
    enginex_midid_process_msg(st, p6_off, 0);
    assert(st->active_mode == SC_PFX_OFF);

    /* Pad 7: Space Reverb */
    uint8_t p7_on[3] = {0x90, 42, 100};
    enginex_midid_process_msg(st, p7_on, 0);
    assert(st->active_mode == SC_PFX_SPACE);
    uint8_t p7_off[3] = {0x80, 42, 0};
    enginex_midid_process_msg(st, p7_off, 0);
    assert(st->active_mode == SC_PFX_OFF);

    /* Pad 8: Pitch Echo */
    uint8_t p8_on[3] = {0x90, 43, 100};
    enginex_midid_process_msg(st, p8_on, 0);
    assert(st->active_mode == SC_PFX_PITCH_ECHO);
    uint8_t p8_off[3] = {0x80, 43, 0};
    enginex_midid_process_msg(st, p8_off, 0);
    assert(st->active_mode == SC_PFX_OFF);

    printf("test_scene_fx: PASS\n");
}

static void test_knobs_and_stems(struct sc_midi_fx_state *st)
{
    /* K1 (CC 1): Wet/Dry mix to 50% (val 64) */
    uint8_t k1[3] = {0xB0, 1, 64};
    enginex_midid_process_msg(st, k1, 0);
    assert(fabsf(st->wet_dry - (64.0f / 127.0f)) < 0.01f);

    /* K2 (CC 2): Filter cutoff */
    uint8_t k2[3] = {0xB0, 2, 127};
    enginex_midid_process_msg(st, k2, 0);
    assert(fabsf(st->filter_cutoff - 1.0f) < 0.01f);

    /* K5 (CC 5): Vocal Stems level to 0 (mute vocal) */
    uint8_t k5[3] = {0xB0, 5, 0};
    enginex_midid_process_msg(st, k5, 0);
    assert(st->stems_level[0] == 0.0f);

    /* K8 (CC 8): Drum Stems level to 100% */
    uint8_t k8[3] = {0xB0, 8, 127};
    enginex_midid_process_msg(st, k8, 0);
    assert(st->stems_level[3] == 1.0f);

    printf("test_knobs_and_stems: PASS\n");
}

static void test_dsp_integration(void)
{
    static struct sc_pioneer_fx fx;
    memset(&fx, 0, sizeof(fx));

    float in_l[512], in_r[512];
    for (int i = 0; i < 512; i++) {
        in_l[i] = sinf((float)i * 0.05f) * 0.8f;
        in_r[i] = cosf((float)i * 0.05f) * 0.8f;
    }

    /* Process through dry (wet_dry = 0.0f) */
    float test_l[512], test_r[512];
    memcpy(test_l, in_l, sizeof(in_l));
    memcpy(test_r, in_r, sizeof(in_r));
    sc_pioneer_fx_planar_ex(&fx, test_l, test_r, 512, SC_PFX_SLIP_ROLL, 0.0f, 1500);

    for (int i = 0; i < 512; i++) {
        assert(fabsf(test_l[i] - in_l[i]) < 1e-4f);
        assert(fabsf(test_r[i] - in_r[i]) < 1e-4f);
    }

    /* Process through full wet (wet_dry = 1.0f) with Roll */
    sc_pioneer_fx_planar_ex(&fx, test_l, test_r, 512, SC_PFX_SLIP_ROLL, 1.0f, 1500);

    /* Verify no NaN or Inf */
    for (int i = 0; i < 512; i++) {
        assert(!isnan(test_l[i]) && !isinf(test_l[i]));
        assert(!isnan(test_r[i]) && !isinf(test_r[i]));
    }

    printf("test_dsp_integration: PASS\n");
}

int main(void)
{
    printf("Running EngineX MIDI FX unit tests...\n");

    struct sc_midi_fx_state test_state;
    memset(&test_state, 0, sizeof(test_state));
    test_state.active_mode = SC_PFX_OFF;
    test_state.wet_dry = 1.0f;
    test_state.stems_level[0] = 1.0f;
    test_state.stems_level[1] = 1.0f;
    test_state.stems_level[2] = 1.0f;
    test_state.stems_level[3] = 1.0f;
    test_state.connected = 1;

    test_initial_state(&test_state);
    test_xpad_rolls(&test_state);
    test_scene_fx(&test_state);
    test_knobs_and_stems(&test_state);
    test_dsp_integration();

    printf("\nALL ENGINE-X MIDI FX TESTS PASSED!\n");
    return 0;
}
