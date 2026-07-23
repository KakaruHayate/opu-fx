#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "OpuFxDSP.h"

const int kNumPresets = 1;

enum EParams {
  // EQ
  kEqLowDb = 0,
  kEqMidFreq,
  kEqMidDb,
  kEqHighDb,
  // Compressor
  kCompThreshold,
  kCompRatio,
  kCompAttack,
  kCompRelease,
  kCompMakeup,
  // Reverb
  kRevRoomSize,
  kRevDamp,
  kWet,
  kDry,
  kRevPreDelay,
  // Global bypass
  kBypass,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class OO_TrackPolish final : public Plugin
{
public:
  OO_TrackPolish(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
#endif

private:
  void UpdateParams();
  opu::dsp::BiquadEQ         mEQ;
  opu::dsp::SimpleCompressor mComp;
  opu::dsp::Freeverb         mReverb;
};
