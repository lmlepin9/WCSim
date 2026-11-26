#ifndef AmBeTaggerWaveformBuilder_h
#define AmBeTaggerWaveformBuilder_h 1


#include "AmBeTaggerSinglePhotoelectronGain.hh"
#include "AmBeTaggerSinglePhotoelectronPulse.hh"
#include "globals.hh"

#include <vector>

namespace AmBeTagger
{
class WaveformBuilder final
{
 public:
  // Amplitudes in volts. Gain = single-PE peak height: 5.8 mV, i.e. 0.90 pC
  // into 50 ohm at the R1398-04 datasheet gain 5.6e6 (1500 V); spread 40%.
  explicit WaveformBuilder(G4double noiseSigma = 0.001,
                           G4double gainMean = 0.0058,
                           G4double gainSigma = 0.0023);
  std::vector<G4double> Build(const std::vector<G4double>& photoelectronTimes) const;

  // Test option: build the waveform as BGOg4Sim's photon_histogram_builder.py
  // (see BuildReference). Off by default.
  void SetReferenceMode(G4bool on) { referenceMode_ = on; }
  G4bool ReferenceMode() const { return referenceMode_; }
  G4int SampleCount() const;
  G4double SampleSpacing() const;
  G4double TriggerTime() const;
  G4double StartTime() const;
  G4double EndTime() const;
  G4double Duration() const;
  G4double SampleTime(G4int sample) const;

 private:
  SinglePhotoelectronPulse singlePE_;
  SinglePhotoelectronGain gain_;
  G4double noiseSigma_ = 0.0;
  G4bool referenceMode_ = false;

  std::vector<G4double> BuildReference(
      const std::vector<G4double>& photoelectronTimes) const;
};
}

#endif
