# OPU-FX

VST3 audio effects derived from [OpenUtau](https://github.com/openutau/OpenUtau)'s per-track post-FX chain (commit [`cfcd654`](https://github.com/openutau/OpenUtau/commit/cfcd6543ba25d5b5054e65b818cc0fb1575e6f17)).

Four plugins based on the original DSP by KakaruHayate:

| Plugin | Description |
|--------|-------------|
| **OO_EQ** | 3-band biquad equaliser (low-shelf / peak / high-shelf) |
| **OO_Comp** | Soft-knee feed-forward compressor / limiter |
| **OO_Reverb** | Schroeder/Moorer reverb (Freeverb) with pre-delay |
| **OO_TrackPolish** | All three in series — EQ → Comp → Reverb |

## Build

```bash
git clone --recursive https://github.com/KakaruHayate/opu-fx.git
cd opu-fx
cmake -B build -DIPLUG2_DIR=<path-to-iplug2>/iPlug2
cmake --build build --config Release
```

## License

MIT — see [LICENSE](LICENSE).

DSP code originally from OpenUtau (MIT). iPlug2 framework (MIT).
