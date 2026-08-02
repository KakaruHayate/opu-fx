/*
 * OPU-FX DSP Engine
 * Ported from OpenUtau commit cfcd654 (C#) → C++17
 * Single-header, header-only library.
 *
 * Effects:
 *   BiquadEQ         — 3-band biquad (RBJ Audio EQ Cookbook)
 *   SimpleCompressor — soft-knee feed-forward, stereo-linked
 *   Freeverb         — Schroeder/Moorer with pre-delay
 *
 * All Process() methods take interleaved float buffers.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace opu::dsp {

inline constexpr double Pi = 3.141592653589793238462643383279502884;

// ── BiquadEQ ─────────────────────────────────────────────────────────
// Three-band biquad equaliser: low-shelf 200 Hz / peak / high-shelf 8 kHz.

struct BiquadEQ {
    struct Params {
        double lowDb   = 0.0;
        double midFreq = 1000.0;
        double midQ    = 0.707;
        double midDb   = 0.0;
        double highDb  = 0.0;
    };

    explicit BiquadEQ(double sampleRate = 44100.0, int channels = 2)
        : sr(sampleRate), ch(channels), bypassed(true) {
        for (int s = 0; s < 3; s++)
            for (int c = 0; c < channels; c++)
                stages[s][c] = {};
    }

    void Reset() {
        for (int s = 0; s < 3; s++)
            for (int c = 0; c < ch; c++)
                stages[s][c] = {};
    }

    void Configure(const Params& p) {
        constexpr double eps = 0.01;
        bypassed = (std::abs(p.lowDb) < eps && std::abs(p.midDb) < eps && std::abs(p.highDb) < eps);
        if (bypassed) return;
        for (int c = 0; c < ch; c++) {
            stages[0][c].SetLowShelf(sr, 200.0, p.lowDb);
            stages[1][c].SetPeak(sr, p.midFreq, p.midQ, p.midDb);
            stages[2][c].SetHighShelf(sr, 8000.0, p.highDb);
        }
    }

    bool IsBypassed() const { return bypassed; }

    void Process(float* buffer, int offset, int numFrames) {
        if (bypassed) return;
        for (int i = 0; i < numFrames; i++) {
            for (int c = 0; c < ch; c++) {
                float x = buffer[offset + i * ch + c];
                x = stages[0][c].Process(x);
                x = stages[1][c].Process(x);
                x = stages[2][c].Process(x);
                buffer[offset + i * ch + c] = x;
            }
        }
    }

private:
    double sr;
    int ch;
    bool bypassed;

    // Direct Form I biquad
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        void Reset() { x1 = x2 = y1 = y2 = 0; }

        float Process(float x) {
            float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }

        // RBJ Audio EQ Cookbook
        void SetPeak(double fs, double f0, double Q, double gainDb) {
            double A  = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * Pi * f0 / fs;
            double cw = std::cos(w0), sw = std::sin(w0);
            double alpha = sw / (2.0 * Q);
            double a0_inv = 1.0 / (1.0 + alpha / A);
            Apply(
                (1.0 + alpha * A) * a0_inv,
                (-2.0 * cw) * a0_inv,
                (1.0 - alpha * A) * a0_inv,
                (2.0 * cw) * a0_inv,
                (-1.0 + alpha / A) * a0_inv);
        }

        void SetLowShelf(double fs, double f0, double gainDb) {
            double A  = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * Pi * f0 / fs;
            double cw = std::cos(w0), sw = std::sin(w0);
            double S = 1.0;
            double alpha = sw / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
            double beta = 2.0 * std::sqrt(A) * alpha;
            double a0_inv = 1.0 / ((A + 1.0) + (A - 1.0) * cw + beta);
            Apply(
                (A * ((A + 1.0) - (A - 1.0) * cw + beta)) * a0_inv,
                (2.0 * A * ((A - 1.0) - (A + 1.0) * cw)) * a0_inv,
                (A * ((A + 1.0) - (A - 1.0) * cw - beta)) * a0_inv,
                (-2.0 * ((A - 1.0) + (A + 1.0) * cw)) * a0_inv,
                ((A + 1.0) + (A - 1.0) * cw - beta) * a0_inv);
        }

        void SetHighShelf(double fs, double f0, double gainDb) {
            double A  = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * Pi * f0 / fs;
            double cw = std::cos(w0), sw = std::sin(w0);
            double S = 1.0;
            double alpha = sw / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
            double beta = 2.0 * std::sqrt(A) * alpha;
            double a0_inv = 1.0 / ((A + 1.0) - (A - 1.0) * cw + beta);
            Apply(
                (A * ((A + 1.0) + (A - 1.0) * cw + beta)) * a0_inv,
                (-2.0 * A * ((A - 1.0) + (A + 1.0) * cw)) * a0_inv,
                (A * ((A + 1.0) + (A - 1.0) * cw - beta)) * a0_inv,
                (2.0 * ((A - 1.0) - (A + 1.0) * cw)) * a0_inv,
                ((A + 1.0) - (A - 1.0) * cw - beta) * a0_inv);
        }

    private:
        void Apply(float b0, float b1, float b2, float a1, float a2) {
            this->b0 = (float)b0; this->b1 = (float)b1; this->b2 = (float)b2;
            this->a1 = (float)a1; this->a2 = (float)a2;
        }
    };

    Biquad stages[3][8]; // [stage][channel], max 8 channels
};

// ── SimpleCompressor ─────────────────────────────────────────────────
// Soft-knee feed-forward, stereo-linked peak detector.

struct SimpleCompressor {
    struct Params {
        double thresholdDb = -18.0;
        double ratio       = 2.0;
        double attackMs    = 10.0;
        double releaseMs   = 120.0;
        double makeupDb    = 2.5;
        double kneeDb      = 6.0;
    };

    explicit SimpleCompressor(double sampleRate = 44100.0, int channels = 2)
        : sr(sampleRate), ch(channels), bypassed(true), env(0.0) {}

    void Reset() { env = 0.0; }

    void Configure(const Params& p) {
        bypassed = (p.ratio <= 1.0001 && std::abs(p.makeupDb) < 0.01);
        threshold = p.thresholdDb;
        slope     = 1.0 / std::max(1.0, p.ratio) - 1.0;
        atkCoef   = std::exp(-1.0 / (std::max(0.05, p.attackMs)  * 0.001 * sr));
        relCoef   = std::exp(-1.0 / (std::max(0.05, p.releaseMs) * 0.001 * sr));
        makeup    = (float)std::pow(10.0, p.makeupDb / 20.0);
        kneeHalf  = (float)(p.kneeDb / 2.0);
    }

    bool IsBypassed() const { return bypassed; }

    void Process(float* buffer, int offset, int numFrames) {
        if (bypassed) return;

        double T = threshold;
        double W = kneeHalf * 2.0;
        double sl = slope;
        double atk = atkCoef;
        double rel = relCoef;
        double e = env;
        float mk = makeup;

        for (int i = 0; i < numFrames; i++) {
            // Stereo-linked peak
            float peak = 0.0f;
            for (int c = 0; c < ch; c++) {
                float v = std::abs(buffer[offset + i * ch + c]);
                if (v > peak) peak = v;
            }

            double det = (double)peak + 1e-12;
            double detDb = 20.0 * std::log10(det);

            // Soft-knee static curve (piecewise quadratic)
            double above = detDb - T;
            double gainDb;
            double kl = -W / 2.0;
            double kh =  W / 2.0;
            if (above <= kl) {
                gainDb = 0.0;
            } else if (above >= kh) {
                gainDb = sl * above;
            } else {
                double k = above + W / 2.0;
                gainDb = sl * (k * k) / (2.0 * W);
            }

            // Envelope follower
            double coef = (gainDb < e) ? atk : rel;
            e = coef * e + (1.0 - coef) * gainDb;

            float linGain = (float)(std::pow(10.0, e / 20.0) * mk);
            for (int c = 0; c < ch; c++) {
                buffer[offset + i * ch + c] *= linGain;
            }
        }
        env = e;
    }

private:
    double sr;
    int ch;
    bool bypassed;

    double threshold;
    double slope;
    double atkCoef, relCoef;
    double makeup;
    double kneeHalf;
    double env; // gain reduction in dB (>= 0)
};

// ── Freeverb ─────────────────────────────────────────────────────────
// Schroeder/Moorer reverb. 8 parallel combs → 4 series allpasses.
// Mono sum → network → stereo spread. Pre-delay on input only.

struct Freeverb {
    struct Params {
        double roomSize    = 0.30;
        double damp        = 0.7;
        double width       = 0.8;
        double wet         = 0.18;
        double dry         = 0.85;
        double preDelayMs  = 12.0;
    };

    static constexpr double FixedGain  = 0.015;
    static constexpr double ScaleDamp  = 0.4;
    static constexpr double ScaleRoom  = 0.28;
    static constexpr double OffsetRoom = 0.7;
    static constexpr int    MaxPreDelayMs = 200;

    explicit Freeverb(double sampleRate = 44100.0)
        : sr(sampleRate), bypassed(true), wet(0.3f), dry(0.7f),
          width(1.0f), preDelaySamples(0), preDelayIdx(0) {

        double scale = sr / 44100.0;

        static constexpr int combL_base[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        static constexpr int apL_base[4]   = { 556,  441,  341,  225};
        static constexpr int stereoSpread  = 23;

        for (int i = 0; i < 8; i++) {
            combsL[i] = CombFilter((int)(combL_base[i] * scale));
            combsR[i] = CombFilter((int)((combL_base[i] + stereoSpread) * scale));
        }
        for (int i = 0; i < 4; i++) {
            apsL[i] = AllpassFilter((int)(apL_base[i] * scale));
            apsR[i] = AllpassFilter((int)((apL_base[i] + stereoSpread) * scale));
            apsL[i].fb = 0.5f;
            apsR[i].fb = 0.5f;
        }

        int maxDelay = std::max(1, (int)(sr * MaxPreDelayMs / 1000));
        preDelayBuf.resize(maxDelay);
    }

    void Reset() {
        for (auto& c : combsL) c.Reset();
        for (auto& c : combsR) c.Reset();
        for (auto& a : apsL) a.Reset();
        for (auto& a : apsR) a.Reset();
        std::fill(preDelayBuf.begin(), preDelayBuf.end(), 0.0f);
        preDelayIdx = 0;
    }

    void Configure(const Params& p) {
        width = (float)std::clamp(p.width, 0.0, 1.0);
        wet   = (float)std::max(0.0, p.wet);
        dry   = (float)std::max(0.0, p.dry);

        float fb = (float)(p.roomSize * ScaleRoom + OffsetRoom);
        float d1 = (float)(p.damp * ScaleDamp);
        for (auto& c : combsL) { c.fb = fb; c.SetDamp(d1); }
        for (auto& c : combsR) { c.fb = fb; c.SetDamp(d1); }

        double pdMs = std::clamp(p.preDelayMs, 0.0, (double)MaxPreDelayMs);
        preDelaySamples = (int)std::round(pdMs * sr / 1000.0);
        if (preDelaySamples >= (int)preDelayBuf.size())
            preDelaySamples = (int)preDelayBuf.size() - 1;

        bypassed = (wet < 1e-4);
    }

    bool IsBypassed() const { return bypassed; }

    void Process(float* buffer, int offset, int numFrames) {
        if (bypassed) return;

        int pdLen   = preDelaySamples;
        int pdSize  = (int)preDelayBuf.size();
        float wet1  = wet * (width / 2.0f + 0.5f);
        float wet2  = wet * ((1.0f - width) / 2.0f);
        float d     = dry;

        for (int i = 0; i < numFrames; i++) {
            int idx = offset + i * 2;
            float inL = buffer[idx];
            float inR = buffer[idx + 1];
            float input = (inL + inR) * (float)FixedGain;

            // Pre-delay
            if (pdLen > 0) {
                preDelayBuf[preDelayIdx] = input;
                int ri = preDelayIdx - pdLen;
                if (ri < 0) ri += pdSize;
                input = preDelayBuf[ri];
                preDelayIdx++;
                if (preDelayIdx == pdSize) preDelayIdx = 0;
            }

            float outL = 0.0f, outR = 0.0f;
            for (int k = 0; k < 8; k++) {
                outL += combsL[k].Process(input);
                outR += combsR[k].Process(input);
            }
            for (int k = 0; k < 4; k++) {
                outL = apsL[k].Process(outL);
                outR = apsR[k].Process(outR);
            }
            buffer[idx]     = outL * wet1 + outR * wet2 + inL * d;
            buffer[idx + 1] = outR * wet1 + outL * wet2 + inR * d;
        }
    }

private:
    double sr;
    int preDelaySamples;
    int preDelayIdx;
    std::vector<float> preDelayBuf;

    float width, wet, dry;
    bool bypassed;

    // Comb with one-pole LPF in feedback
    struct CombFilter {
        float* buf = nullptr;
        int size = 0, idx = 0;
        float filtStore = 0.0f;
        float damp1 = 0.0f, damp2 = 0.0f;
        float fb = 0.5f;

        CombFilter() = default;
        explicit CombFilter(int sz) : size(std::max(1, sz)) {
            buf = new float[size];
            std::fill(buf, buf + size, 0.0f);
        }
        ~CombFilter() { delete[] buf; }
        CombFilter(const CombFilter&) = delete;
        CombFilter& operator=(const CombFilter&) = delete;
        CombFilter(CombFilter&& o) noexcept : buf(o.buf), size(o.size), idx(o.idx),
            filtStore(o.filtStore), damp1(o.damp1), damp2(o.damp2), fb(o.fb) {
            o.buf = nullptr; o.size = 0;
        }
        CombFilter& operator=(CombFilter&& o) noexcept {
            if (this != &o) {
                delete[] buf;
                buf = o.buf;
                size = o.size;
                idx = o.idx;
                filtStore = o.filtStore;
                damp1 = o.damp1;
                damp2 = o.damp2;
                fb = o.fb;
                o.buf = nullptr;
                o.size = 0;
            }
            return *this;
        }

        void Reset() {
            if (buf == nullptr) return;
            std::fill(buf, buf + size, 0.0f);
            filtStore = 0.0f;
            idx = 0;
        }

        void SetDamp(float d) { damp1 = d; damp2 = 1.0f - d; }

        float Process(float x) {
            float y = buf[idx];
            filtStore = y * damp2 + filtStore * damp1;
            buf[idx]  = x + filtStore * fb;
            idx++;
            if (idx == size) idx = 0;
            return y;
        }
    };

    // Schroeder allpass section
    struct AllpassFilter {
        float* buf = nullptr;
        int size = 0, idx = 0;
        float fb = 0.5f;

        AllpassFilter() = default;
        explicit AllpassFilter(int sz) : size(std::max(1, sz)) {
            buf = new float[size];
            std::fill(buf, buf + size, 0.0f);
        }
        ~AllpassFilter() { delete[] buf; }
        AllpassFilter(const AllpassFilter&) = delete;
        AllpassFilter& operator=(const AllpassFilter&) = delete;
        AllpassFilter(AllpassFilter&& o) noexcept : buf(o.buf), size(o.size),
            idx(o.idx), fb(o.fb) { o.buf = nullptr; o.size = 0; }
        AllpassFilter& operator=(AllpassFilter&& o) noexcept {
            if (this != &o) {
                delete[] buf;
                buf = o.buf;
                size = o.size;
                idx = o.idx;
                fb = o.fb;
                o.buf = nullptr;
                o.size = 0;
            }
            return *this;
        }

        void Reset() {
            if (buf == nullptr) return;
            std::fill(buf, buf + size, 0.0f);
            idx = 0;
        }

        float Process(float x) {
            float bufOut = buf[idx];
            float y = -x + bufOut;
            buf[idx] = x + bufOut * fb;
            idx++;
            if (idx == size) idx = 0;
            return y;
        }
    };

    CombFilter combsL[8], combsR[8];
    AllpassFilter apsL[4], apsR[4];
};

} // namespace opu::dsp
