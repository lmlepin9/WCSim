#ifndef AmBeTaggerWaveformBuilder_h
#define AmBeTaggerWaveformBuilder_h 1


#include "globals.hh"

#include <vector>

namespace AmBeTagger
{
// Tagger PMT waveform, built as BGOg4Sim's photon_histogram_builder.py does
// (see Build). Amplitudes in volts.
class WaveformBuilder final
{
 public:
  explicit WaveformBuilder(G4double noiseSigma = 0.001);
  std::vector<G4double> Build(const std::vector<G4double>& photoelectronTimes) const;

  G4int SampleCount() const;
  G4double SampleSpacing() const;
  G4double TriggerTime() const;
  G4double StartTime() const;

 private:
  G4double noiseSigma_ = 0.0;
};
}

#endif
