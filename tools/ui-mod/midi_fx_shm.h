/* EngineX OS - Real-time MIDI FX Shared Memory Interface (#180).
 *
 * Provides a lock-free, zero-syscall shared memory bridge between
 * USB MIDI controllers (such as the Akai LPD8) and Engine's audio DSP hook.
 */
#ifndef SC_MIDI_FX_SHM_H
#define SC_MIDI_FX_SHM_H

#include <stdint.h>

#define SC_MIDI_FX_SHM_NAME "/enginex_midi_fx"
#define SC_MIDI_FX_SHM_PATH "/dev/shm/enginex_midi_fx"

/* X-Pad & Scene FX definitions matching Pioneer RMX-1000 style */
#define SC_MIDI_PAD_ROLL_1_8  1   /* Pad 1: 1/8 beat slice roll (approx 1500 frames @ 48kHz / 120BPM) */
#define SC_MIDI_PAD_ROLL_1_4  2   /* Pad 2: 1/4 beat slice roll (approx 3000 frames) */
#define SC_MIDI_PAD_ROLL_1_2  3   /* Pad 3: 1/2 beat slice roll (approx 6000 frames) */
#define SC_MIDI_PAD_ROLL_1_1  4   /* Pad 4: 1 beat slice roll (approx 12000 frames) */
#define SC_MIDI_PAD_SPIRAL    5   /* Pad 5: DJM Spiral pitch-feedback delay */
#define SC_MIDI_PAD_DUB_ECHO  6   /* Pad 6: Sound Color FX Dub Echo */
#define SC_MIDI_PAD_SPACE     7   /* Pad 7: Reverb / Space diffuser */
#define SC_MIDI_PAD_ECHO_OUT  8   /* Pad 8: Echo Out release tail */

struct sc_midi_fx_state {
    uint32_t active_mode;       /* Active Pioneer FX mode (SC_PFX_OFF, SC_PFX_SLIP_ROLL, etc.) */
    uint32_t roll_frames;       /* Frame count for roll slice (e.g. 1500, 3000, 6000, 12000) */
    float wet_dry;              /* Wet/Dry mix: 0.0f (100% dry) to 1.0f (100% wet) */
    float filter_cutoff;        /* Isolator filter: -1.0f (LPF) .. 0.0f (flat) .. +1.0f (HPF) */
    float resonance;            /* Filter resonance / delay feedback: 0.0f to 1.0f */
    float pitch_mod;            /* Pitch ratio for pitch-shifting FX: 0.5f to 2.0f */
    float stems_level[4];       /* [0]=Vocal, [1]=Melody, [2]=Bass, [3]=Drums (0.0f to 1.0f) */
    uint32_t pad_pressed_mask;  /* Bitmask of currently held pads (bit 0 = Pad 1, bit 7 = Pad 8) */
    uint32_t connected;         /* 1 if a MIDI controller daemon is active, 0 otherwise */
    uint32_t seq;               /* Monotonically increasing sequence counter */
};

#endif /* SC_MIDI_FX_SHM_H */
