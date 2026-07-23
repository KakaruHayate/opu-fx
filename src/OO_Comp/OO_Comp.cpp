#include "OO_Comp.h"
#include "IPlug_include_in_plug_src.h"

using namespace iplug;
using namespace igraphics;

OO_Comp::OO_Comp(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumPresets))
{
  GetParam(kParamThreshold)->InitDouble("Threshold", -18.0, -40.0, 0.0, 0.1, "dB");
  GetParam(kParamRatio)->InitDouble("Ratio", 2.0, 1.0, 20.0, 0.1, "");
  GetParam(kParamAttack)->InitDouble("Attack", 10.0, 0.05, 100.0, 0.1, "ms");
  GetParam(kParamRelease)->InitDouble("Release", 120.0, 10.0, 500.0, 1.0, "ms");
  GetParam(kParamMakeup)->InitDouble("Makeup", 2.5, 0.0, 24.0, 0.1, "dB");
  GetParam(kParamBypass)->InitBool("Bypass", false);

  mComp = opu::dsp::SimpleCompressor(GetSampleRate(), 2);
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
    pGraphics->AttachControl(new ITextControl(inner.GetFromTop(30.f), "OPU COMP", IText(20, COLOR_WHITE)));

    const float colX = inner.L + 40.f;
    const float colW = inner.W() - 80.f;

    auto makeSliderRow = [&](float y, const char* label, int paramID) {
      IRECT row = IRECT::MakeXYWH(colX, y, colW, 55.f);
      pGraphics->AttachControl(new ITextControl(row.GetFromTop(16.f), label, IText(12)));
      pGraphics->AttachControl(new IVSliderControl(row.GetFromBottom(40.f), paramID, label, DEFAULT_STYLE, true, EDirection::Horizontal, DEFAULT_GEARING, 8.f, 4.f));
    };

    makeSliderRow(60.f, "Threshold", kParamThreshold);
    makeSliderRow(130.f, "Ratio", kParamRatio);
    makeSliderRow(200.f, "Attack", kParamAttack);
    makeSliderRow(270.f, "Release", kParamRelease);
    makeSliderRow(340.f, "Makeup", kParamMakeup);

    const IRECT bypassArea = IRECT::MakeXYWH(colX, inner.B - 50.f, 120.f, 30.f);
    pGraphics->AttachControl(new IVToggleControl(bypassArea, kParamBypass, "", DEFAULT_STYLE, "OFF", "ON"));
  };
}

#if IPLUG_DSP
void OO_Comp::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const int nIn = NInChansConnected();
  const int nOut = NOutChansConnected();
  const int nCh = std::min(nIn, nOut);

  for (int ch = 0; ch < nCh; ch++) {
    memcpy(outputs[ch], inputs[ch], nFrames * sizeof(sample));
  }

  if (GetParam(kParamBypass)->GetBool() || mComp.IsBypassed()) return;

  std::vector<float> buf(nFrames * nCh);
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      buf[i * nCh + ch] = (float)outputs[ch][i];
    }
  }
  mComp.Process(buf.data(), 0, nFrames);
  for (int i = 0; i < nFrames; i++) {
    for (int ch = 0; ch < nCh; ch++) {
      outputs[ch][i] = buf[i * nCh + ch];
    }
  }
}
#endif

void OO_Comp::OnReset() { mComp.Reset(); }

void OO_Comp::OnParamChange(int paramIdx) {
  if (paramIdx == kParamBypass) {
    if (GetParam(kParamBypass)->GetBool()) {
      mComp.Configure(opu::dsp::SimpleCompressor::Params{});
    } else {
      UpdateParams();
    }
    return;
  }
  if (GetParam(kParamBypass)->GetBool()) return;
  UpdateParams();
}

void OO_Comp::UpdateParams() {
  opu::dsp::SimpleCompressor::Params p;
  p.thresholdDb = GetParam(kParamThreshold)->Value();
  p.ratio = GetParam(kParamRatio)->Value();
  p.attackMs = GetParam(kParamAttack)->Value();
  p.releaseMs = GetParam(kParamRelease)->Value();
  p.makeupDb = GetParam(kParamMakeup)->Value();
  p.kneeDb = 6.0;
  mComp.Configure(p);
}
