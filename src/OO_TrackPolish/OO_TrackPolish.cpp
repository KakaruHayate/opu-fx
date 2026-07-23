#include "OO_TrackPolish.h"
#include "IPlug_include_in_plug_src.h"

using namespace iplug;
using namespace igraphics;

OO_TrackPolish::OO_TrackPolish(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumPresets))
{
  // EQ
  GetParam(kEqLowDb)->InitDouble("EQ Low", 0.0, -12.0, 12.0, 0.1, "dB");
  GetParam(kEqMidFreq)->InitDouble("EQ Mid Frq", 3000.0, 200.0, 6000.0, 10.0, "Hz");
  GetParam(kEqMidDb)->InitDouble("EQ Mid", 1.5, -12.0, 12.0, 0.1, "dB");
  GetParam(kEqHighDb)->InitDouble("EQ High", 3.0, -12.0, 12.0, 0.1, "dB");
  // Compressor
  GetParam(kCompThreshold)->InitDouble("Thresh", -18.0, -40.0, 0.0, 0.1, "dB");
  GetParam(kCompRatio)->InitDouble("Ratio", 2.0, 1.0, 20.0, 0.1, "");
  GetParam(kCompAttack)->InitDouble("Attack", 10.0, 0.05, 100.0, 0.1, "ms");
  GetParam(kCompRelease)->InitDouble("Release", 120.0, 10.0, 500.0, 1.0, "ms");
  GetParam(kCompMakeup)->InitDouble("Makeup", 2.5, 0.0, 24.0, 0.1, "dB");
  // Reverb
  GetParam(kRevRoomSize)->InitDouble("Room Size", 0.30, 0.0, 1.0, 0.01, "");
  GetParam(kRevDamp)->InitDouble("Damp", 0.7, 0.0, 1.0, 0.01, "");
  GetParam(kWet)->InitDouble("Wet", 0.18, 0.0, 1.0, 0.01, "");
  GetParam(kDry)->InitDouble("Dry", 0.85, 0.0, 1.0, 0.01, "");
  GetParam(kRevPreDelay)->InitDouble("Pre-Dly", 12.0, 0.0, 200.0, 1.0, "ms");
  // Global
  GetParam(kBypass)->InitBool("Bypass", false);

  mEQ = opu::dsp::BiquadEQ(GetSampleRate(), 2);
  mComp = opu::dsp::SimpleCompressor(GetSampleRate(), 2);
  mReverb = opu::dsp::Freeverb(GetSampleRate());
  UpdateParams();

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->EnableMouseOver(true);

    const IRECT b = pGraphics->GetBounds();
    const IRECT inner = b.GetPadded(-20.f);
    pGraphics->AttachControl(new ITextControl(inner.GetFromTop(28.f), "OPU TRACK POLISH", IText(18, COLOR_WHITE)));

    // Three columns separated by 8px gaps
    const float totalW = inner.W() - 40.f;
    const float colW = (totalW - 16.f) / 3.f;
    const float rowH = 42.f;
    const float labelH = 16.f;
    const float sliderH = 28.f;
    const float startY = 60.f;

    auto makeRow = [&](float y, const char* label, int paramID, float colX) {
      IRECT row = IRECT::MakeXYWH(colX, y, colW, rowH);
      pGraphics->AttachControl(new ITextControl(row.GetFromTop(labelH), label, IText(11)));
      pGraphics->AttachControl(new IVSliderControl(row.GetFromBottom(sliderH), paramID, label, DEFAULT_STYLE, true, EDirection::Horizontal, DEFAULT_GEARING, 6.f, 3.f));
    };

    // EQ column
    float x0 = inner.L + 20.f;
    makeRow(startY,               "Low dB",      kEqLowDb,    x0);
    makeRow(startY + rowH,        "Mid Freq",    kEqMidFreq,  x0);
    makeRow(startY + rowH * 2.f,  "Mid dB",      kEqMidDb,    x0);
    makeRow(startY + rowH * 3.f,  "High dB",     kEqHighDb,   x0);

    // Comp column
    float x1 = x0 + colW + 8.f;
    makeRow(startY,               "Threshold",  kCompThreshold, x1);
    makeRow(startY + rowH,        "Ratio",      kCompRatio,     x1);
    makeRow(startY + rowH * 2.f,  "Attack",     kCompAttack,    x1);
    makeRow(startY + rowH * 3.f,  "Release",    kCompRelease,   x1);
    makeRow(startY + rowH * 4.f,  "Makeup",     kCompMakeup,    x1);

    // Reverb column
    float x2 = x1 + colW + 8.f;
    makeRow(startY,               "Room Size",   kRevRoomSize, x2);
    makeRow(startY + rowH,        "Damp",        kRevDamp,     x2);
    makeRow(startY + rowH * 2.f,  "Wet",         kWet,         x2);
    makeRow(startY + rowH * 3.f,  "Dry",         kDry,         x2);
    makeRow(startY + rowH * 4.f,  "Pre-Delay",   kRevPreDelay, x2);

    // Global bypass
    const IRECT bypassArea = IRECT::MakeXYWH(inner.L + 20.f, inner.B - 40.f, 120.f, 28.f);
    pGraphics->AttachControl(new IVToggleControl(bypassArea, kBypass, "", DEFAULT_STYLE, "OFF", "ON"));
  };
}

#if IPLUG_DSP
void OO_TrackPolish::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const int nIn = NInChansConnected();
  const int nOut = NOutChansConnected();
  const int nCh = std::min(nIn, nOut);

  for (int ch = 0; ch < nCh; ch++) {
    memcpy(outputs[ch], inputs[ch], nFrames * sizeof(sample));
  }

  if (GetParam(kBypass)->GetBool()) return;

  // De-interleave
  std::vector<float> buf(nFrames * nCh);
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      buf[i * nCh + ch] = (float)outputs[ch][i];
    }
  }

  if (!mEQ.IsBypassed())     mEQ.Process(buf.data(), 0, nFrames);
  if (!mComp.IsBypassed())   mComp.Process(buf.data(), 0, nFrames);
  if (!mReverb.IsBypassed()) mReverb.Process(buf.data(), 0, nFrames);

  // Re-interleave
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      outputs[ch][i] = buf[i * nCh + ch];
    }
  }
}
#endif

void OO_TrackPolish::OnReset() {
  mEQ.Reset();
  mComp.Reset();
  mReverb.Reset();
}

void OO_TrackPolish::OnParamChange(int paramIdx) {
  if (GetParam(kBypass)->GetBool()) return;
  UpdateParams();
}

void OO_TrackPolish::UpdateParams() {
  // EQ
  opu::dsp::BiquadEQ::Params eqP;
  eqP.lowDb = (float)GetParam(kEqLowDb)->Value();
  eqP.midFreq = GetParam(kEqMidFreq)->Value();
  eqP.midQ = 0.707;
  eqP.midDb = (float)GetParam(kEqMidDb)->Value();
  eqP.highDb = (float)GetParam(kEqHighDb)->Value();
  mEQ.Configure(eqP);

  // Comp
  opu::dsp::SimpleCompressor::Params compP;
  compP.thresholdDb = GetParam(kCompThreshold)->Value();
  compP.ratio = GetParam(kCompRatio)->Value();
  compP.attackMs = GetParam(kCompAttack)->Value();
  compP.releaseMs = GetParam(kCompRelease)->Value();
  compP.makeupDb = GetParam(kCompMakeup)->Value();
  compP.kneeDb = 6.0;
  mComp.Configure(compP);

  // Reverb
  opu::dsp::Freeverb::Params revP;
  revP.roomSize = GetParam(kRevRoomSize)->Value();
  revP.damp = GetParam(kRevDamp)->Value();
  revP.wet = GetParam(kWet)->Value();
  revP.dry = GetParam(kDry)->Value();
  revP.width = 0.8f;
  revP.preDelayMs = GetParam(kRevPreDelay)->Value();
  mReverb.Configure(revP);
}
