#include "OO_EQ.h"
#include "IPlug_include_in_plug_src.h"

using namespace iplug;
using namespace igraphics;

OO_EQ::OO_EQ(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumPresets))
{
  GetParam(kParamLowDb)->InitDouble("Low dB", 0.0, -12.0, 12.0, 0.1, "dB");
  GetParam(kParamMidFreq)->InitDouble("Mid Freq", 3000.0, 200.0, 6000.0, 10.0, "Hz");
  GetParam(kParamMidDb)->InitDouble("Mid dB", 1.5, -12.0, 12.0, 0.1, "dB");
  GetParam(kParamHighDb)->InitDouble("High dB", 3.0, -12.0, 12.0, 0.1, "dB");
  GetParam(kParamBypass)->InitBool("Bypass", false);

  mEQ = opu::dsp::BiquadEQ(GetSampleRate(), 2);
  UpdateParams();

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->EnableMouseOver(true);

    const IRECT b = pGraphics->GetBounds();
    const IRECT inner = b.GetPadded(-30.f);

    // Title
    pGraphics->AttachControl(new ITextControl(inner.GetFromTop(30.f), "OPU EQ", IText(20, COLOR_WHITE)));

    // Column: sliders stacked vertically
    const float colX = inner.L + 40.f;
    const float colW = inner.W() - 80.f;

    auto makeSliderRow = [&](float y, const char* label, int paramID) {
      IRECT row = IRECT::MakeXYWH(colX, y, colW, 55.f);
      pGraphics->AttachControl(new ITextControl(row.GetFromTop(16.f), label, IText(12)));
      pGraphics->AttachControl(new IVSliderControl(row.GetFromBottom(40.f), paramID, label, DEFAULT_STYLE, true, EDirection::Horizontal, DEFAULT_GEARING, 8.f, 4.f));
    };

    makeSliderRow(60.f, "Low (dB)", kParamLowDb);
    makeSliderRow(130.f, "Mid Freq (Hz)", kParamMidFreq);
    makeSliderRow(200.f, "Mid (dB)", kParamMidDb);
    makeSliderRow(270.f, "High (dB)", kParamHighDb);

    // Bypass at bottom
    const IRECT bypassArea = IRECT::MakeXYWH(colX, inner.B - 50.f, 120.f, 30.f);
    pGraphics->AttachControl(new IVToggleControl(bypassArea, kParamBypass, "", DEFAULT_STYLE, "OFF", "ON"));
  };
}

#if IPLUG_DSP
void OO_EQ::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const int nIn = NInChansConnected();
  const int nOut = NOutChansConnected();
  const int nCh = std::min(nIn, nOut);

  for (int ch = 0; ch < nCh; ch++) {
    memcpy(outputs[ch], inputs[ch], nFrames * sizeof(sample));
  }

  if (GetParam(kParamBypass)->GetBool() || mEQ.IsBypassed()) return;

  // De-interleave
  std::vector<float> buf(nFrames * nCh);
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      buf[i * nCh + ch] = (float)outputs[ch][i];
    }
  }
  mEQ.Process(buf.data(), 0, nFrames);
  // Re-interleave
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      outputs[ch][i] = buf[i * nCh + ch];
    }
  }
}
#endif

void OO_EQ::OnReset() {
  mEQ.Reset();
}

void OO_EQ::OnParamChange(int paramIdx) {
  if (paramIdx == kParamBypass) {
    if (GetParam(kParamBypass)->GetBool()) {
      mEQ.Configure(opu::dsp::BiquadEQ::Params{});
    } else {
      UpdateParams();
    }
    return;
  }
  if (GetParam(kParamBypass)->GetBool()) return;
  UpdateParams();
}

void OO_EQ::UpdateParams() {
  opu::dsp::BiquadEQ::Params p;
  p.lowDb = (float)GetParam(kParamLowDb)->Value();
  p.midFreq = GetParam(kParamMidFreq)->Value();
  p.midQ = 0.707;
  p.midDb = (float)GetParam(kParamMidDb)->Value();
  p.highDb = (float)GetParam(kParamHighDb)->Value();
  mEQ.Configure(p);
}
