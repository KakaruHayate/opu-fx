#include "OO_Reverb.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;

OO_Reverb::OO_Reverb(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamRoomSize)->InitDouble("Room Size", 0.30, 0.0, 1.0, 0.01, "");
  GetParam(kParamDamp)->InitDouble("Damp", 0.7, 0.0, 1.0, 0.01, "");
  GetParam(kWet)->InitDouble("Wet", 0.18, 0.0, 1.0, 0.01, "");
  GetParam(kDry)->InitDouble("Dry", 0.85, 0.0, 1.0, 0.01, "");
  GetParam(kParamPreDelay)->InitDouble("Pre-Delay", 12.0, 0.0, 200.0, 1.0, "ms");
  GetParam(kParamBypass)->InitBool("Bypass", false);

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
    const IRECT inner = b.GetPadded(-30.f);
    pGraphics->AttachControl(new ITextControl(inner.GetFromTop(30.f), "OPU REVERB", IText(20, COLOR_WHITE)));

    const float colX = inner.L + 40.f;
    const float colW = inner.W() - 80.f;

    auto makeSliderRow = [&](float y, const char* label, int paramID) {
      IRECT row = IRECT::MakeXYWH(colX, y, colW, 55.f);
      pGraphics->AttachControl(new ITextControl(row.GetFromTop(16.f), label, IText(12)));
      pGraphics->AttachControl(new IVSliderControl(row.GetFromBottom(40.f), paramID, label, DEFAULT_STYLE, true, EDirection::Horizontal, DEFAULT_GEARING, 8.f, 4.f));
    };

    makeSliderRow(60.f, "Room Size", kParamRoomSize);
    makeSliderRow(130.f, "Damp", kParamDamp);
    makeSliderRow(200.f, "Wet", kWet);
    makeSliderRow(270.f, "Dry", kDry);
    makeSliderRow(340.f, "Pre-Delay", kParamPreDelay);

    const IRECT bypassArea = IRECT::MakeXYWH(colX, inner.B - 50.f, 120.f, 30.f);
    pGraphics->AttachControl(new IVToggleControl(bypassArea, kParamBypass, "", DEFAULT_STYLE, "OFF", "ON"));
  };
}

#if IPLUG_DSP
void OO_Reverb::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const int nIn = NInChansConnected();
  const int nOut = NOutChansConnected();
  const int nCh = std::min(nIn, nOut);

  for (int ch = 0; ch < nCh; ch++) {
    memcpy(outputs[ch], inputs[ch], nFrames * sizeof(sample));
  }

  if (GetParam(kParamBypass)->Bool() || mReverb.IsBypassed()) return;

  std::vector<float> buf(nFrames * nCh);
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      buf[i * nCh + ch] = (float)outputs[ch][i];
    }
  }
  mReverb.Process(buf.data(), 0, nFrames);
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      outputs[ch][i] = buf[i * nCh + ch];
    }
  }
}
#endif

void OO_Reverb::OnReset() { mReverb.Reset(); }

void OO_Reverb::OnParamChange(int paramIdx) {
  if (paramIdx == kParamBypass) {
    if (GetParam(kParamBypass)->Bool()) {
      mReverb.Configure(opu::dsp::Freeverb::Params{});
    } else {
      UpdateParams();
    }
    return;
  }
  if (GetParam(kParamBypass)->Bool()) return;
  UpdateParams();
}

void OO_Reverb::UpdateParams() {
  opu::dsp::Freeverb::Params p;
  p.roomSize = GetParam(kParamRoomSize)->Value();
  p.damp = GetParam(kParamDamp)->Value();
  p.wet = GetParam(kWet)->Value();
  p.dry = GetParam(kDry)->Value();
  p.width = 0.8f;
  p.preDelayMs = GetParam(kParamPreDelay)->Value();
  mReverb.Configure(p);
}
