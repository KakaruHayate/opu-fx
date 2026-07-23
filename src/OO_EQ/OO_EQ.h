#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "OpuFxDSP.h"

const int kNumPresets = 1;

enum EParams {
  kParamLowDb = 0,
  kParamMidFreq,
  kParamMidDb,
  kParamHighDb,
  kParamBypass,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class OO_EQ final : public Plugin
{
public:
  OO_EQ(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
#endif

private:
  void UpdateParams();
  opu::dsp::BiquadEQ mEQ;
};
