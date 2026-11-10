# EngineX MIDI FX Daemon (`enginex_midid`) - Akai LPD8 as Pioneer RMX-1000

Daemon and real-time DSP bridge that turns an **Akai LPD8** (or any class-compliant USB MIDI pad controller) into a **Pioneer RMX-1000** style performance remix station directly connected to the Engine OS audio engine.

---

## Architecture

```
[ Akai LPD8 USB MIDI ]
         │ (Class-compliant USB MIDI /dev/snd/midiC*D*)
         ▼
 ┌────────────────────────┐
 │   enginex_midid daemon │  (User space, nice -10)
 └───────────┬────────────┘
             │ (Atomic lock-free POSIX shared memory: /dev/shm/enginex_midi_fx)
             ▼
 ┌────────────────────────┐
 │   engine_ui_hook.c     │  (Audio thread SCHED_RR 49, 96 kHz)
 │   pioneer_fx.h DSP     │  - Zero syscalls, zero locks, zero malloc
 └────────────────────────┘
```

The daemon reads raw MIDI packets from `/dev/snd/midiC*D*` and writes parameters atomically into a POSIX shared memory buffer (`struct sc_midi_fx_state`). The audio hook reads these atomic parameters in its master audio output callback without taking locks or invoking syscalls, ensuring glitch-free audio performance.

---

## Akai LPD8 Hardware Mapping

### Pads: X-Pad FX & Scene FX (Momentary Touch)
*Pressing a pad activates the effect; releasing it returns smoothly to dry audio.*

| Pad | MIDI Note (Default) | Effect Type | Description |
|:---:|:-------------------:|:-----------:|:------------|
| **PAD 1** | Note 36 (C1) | **X-Pad Roll 1/8** | High-speed 1/8 beat glitch/stutter roll |
| **PAD 2** | Note 37 (C#1) | **X-Pad Roll 1/4** | 1/4 beat loop roll |
| **PAD 3** | Note 38 (D1) | **X-Pad Roll 1/2** | 1/2 beat loop roll |
| **PAD 4** | Note 39 (D#1) | **X-Pad Roll 1/1** | 1 beat (full bar) slip roll |
| **PAD 5** | Note 40 (E1) | **Scene FX: Spiral** | Pioneer DJM spiral pitch-feedback delay |
| **PAD 6** | Note 41 (F1) | **Scene FX: Dub Echo** | Sound Color FX tape-saturated resonant echo |
| **PAD 7** | Note 42 (F#1) | **Scene FX: Space** | Diffusive studio space reverb |
| **PAD 8** | Note 43 (G1) | **Scene FX: Pitch Echo**| Dual harmonic pitch-shifted delay (+5th / +8ve) |

> **Roll Trill Support**: You can slide fingers between pads (e.g., from Pad 4 down to Pad 1) for progressive build-up rolls. The daemon tracks all held pads and switches smoothly.

---

### Knobs: FX Isolator, Modulation & Stems
*Continuous 360-degree potentiometers / rotary encoders.*

| Knob | MIDI CC | Parameter | Range & Behavior |
|:----:|:-------:|:---------:|:-----------------|
| **K1** | CC 1 | **Wet / Dry Mix** | `0%` (Fully Dry) to `100%` (Fully Wet) |
| **K2** | CC 2 | **Isolator Filter** | Center = Flat bypass; Left = LPF cutoff; Right = HPF cutoff |
| **K3** | CC 3 | **Resonance / Feedback**| Feedback decay for Echo / Spiral / Reverb size |
| **K4** | CC 4 | **Pitch Ratio** | `0.5x` (-1 octave) to `2.0x` (+1 octave) pitch modulation |
| **K5** | CC 5 | **Stems: Vocals** | `0.0` (Mute) to `1.0` (Full level) |
| **K6** | CC 6 | **Stems: Melody** | `0.0` (Mute) to `1.0` (Full level) |
| **K7** | CC 7 | **Stems: Bass** | `0.0` (Mute) to `1.0` (Full level) |
| **K8** | CC 8 | **Stems: Drums** | `0.0` (Mute) to `1.0` (Full level) |

---

## Build and Testing

### 1. Build and Run Test Suite
```bash
cd tools/enginex-midid
./build.sh
```

### 2. Standalone Simulation Mode
Run without physical hardware to verify shared memory operation:
```bash
./enginex_midid --simulate
```

### 3. Check Real-Time Shared Memory State
```bash
./enginex_midid --status
```

Output:
```text
=== EngineX MIDI FX State (RMX-1000) ===
  Connected:    YES
  Active FX:    SLIP ROLL (mode 3)
  Roll Frames:  1500 frames
  Wet / Dry:    80.3%
  Filter:       0.00
  Resonance:    0.50
  Pitch Ratio:  1.00x
  Stems Levels: Vocal=1.00  Melody=1.00  Bass=1.00  Drums=1.00
  Pad Mask:     0x01
  Seq counter:  3
```

---

## Running on Hardware

When plugged into the USB port of a Denon DJ SC5000 / SC6000 / PRIME 4 player running EngineX OS, the kernel module `snd-usb-audio` registers the device as `/dev/snd/midiC1D0`.

To start the daemon manually:
```bash
/data/sc5000-custom/bin/enginex_midid
```

Or enable the included systemd service:
```bash
cp tools/enginex-midid/enginex-midid.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now enginex-midid.service
```
