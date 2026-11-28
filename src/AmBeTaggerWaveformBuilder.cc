#include "AmBeTaggerWaveformBuilder.hh"
#include "AmBeTaggerSinglePhotoelectronPulse.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace
{
constexpr G4int kSampleCount = 1500;
const G4double kSampleSpacing = 2.0 * ns;
const G4double kTriggerTime = 700.0 * ns;

// BGOg4Sim photon_histogram_builder.py waveform parameters
const G4double kRefGainMean = 0.005;     // V per PE: AREA of the unit-area pulse
const G4double kRefGainSigma = 0.0006;   // V, per sqrt(PE) in a sample
const G4double kRefTriggerThreshold = 0.010;  // V
// The Python script samples its pulse template at 1, 3, 5, ..., 75 ns after
// the pulse start (spr_time - 23 with spr_time = arange(-50, 100, 2)) and
// normalises those 38 values to unit sum.
constexpr G4int kRefTemplateLength = 38;
}

namespace AmBeTagger
{

WaveformBuilder::WaveformBuilder(G4double noiseSigma)
    : noiseSigma_(noiseSigma)
{
}

// BGOg4Sim photon_histogram_builder.py, step by step:
//  1. PE times are histogrammed in the 2 ns samples of the window.
//  2. Each non-empty sample with N PE gets Normal(N*5 mV, sqrt(N)*0.6 mV)
//     (can be negative, as in the script).
//  3. That is convolved with the unit-sum pulse template, so one PE carries
//     5 mV x sample of area (peak ~1.2 mV). The template starts one transit
//     time after the PE sample (26 ns, rounded up to whole samples).
//  4. Gaussian noise (noiseSigma_, 1 mV) is added to every sample.
//  5. If the waveform exceeds 10 mV, it is shifted so that the first sample
//     above threshold sits at 700 ns (the trigger time); the samples shifted
//     in get fresh noise. Untriggered waveforms are stored unshifted (BGOg4Sim
//     does not write them at all).
// The time axis of a triggered waveform is therefore trigger-aligned, not the
// global time.
std::vector<G4double> WaveformBuilder::Build(
    const std::vector<G4double>& photoelectronTimes) const
{
  const G4int n = SampleCount();
  const G4double dt = SampleSpacing();

  std::vector<G4double> counts(n, 0.0);
  for (std::size_t i = 0; i < photoelectronTimes.size(); ++i) {
    const G4int k =
        static_cast<G4int>(std::floor((photoelectronTimes[i] - StartTime()) / dt));
    if (k >= 0 && k < n) counts[k] += 1.0;
  }

  std::vector<G4double> smeared(n, 0.0);
  for (G4int k = 0; k < n; ++k) {
    if (counts[k] > 0.0) {
      smeared[k] = G4RandGauss::shoot(counts[k] * kRefGainMean,
                                      std::sqrt(counts[k]) * kRefGainSigma);
    }
  }

  std::vector<G4double> pulse(kRefTemplateLength);
  G4double sum = 0.0;
  for (G4int m = 0; m < kRefTemplateLength; ++m) {
    pulse[m] = SinglePhotoelectronPulse::Shape((2 * m + 1) * ns);
    sum += pulse[m];
  }
  for (G4double& v : pulse) v /= sum;
  const G4int offset = static_cast<G4int>(std::ceil(SinglePhotoelectronPulse::TransitTime() / dt));

  std::vector<G4double> waveform(n, 0.0);
  for (G4int k = 0; k < n; ++k) {
    G4double v = 0.0;
    for (G4int m = 0; m < kRefTemplateLength; ++m) {
      const G4int j = k - offset - m;
      if (j < 0) break;
      v += smeared[j] * pulse[m];
    }
    waveform[k] = v + (noiseSigma_ > 0.0 ? G4RandGauss::shoot(0.0, noiseSigma_) : 0.0);
  }

  const std::vector<G4double>::const_iterator cross =
      std::find_if(waveform.begin(), waveform.end(),
                   [](G4double v) { return v > kRefTriggerThreshold; });
  if (cross == waveform.end()) {
    return waveform;
  }
  const G4int shift = static_cast<G4int>(TriggerTime() / dt)
      - static_cast<G4int>(cross - waveform.begin());
  std::vector<G4double> shifted(n, 0.0);
  for (G4int k = 0; k < n; ++k) {
    shifted[k] = noiseSigma_ > 0.0 ? G4RandGauss::shoot(0.0, noiseSigma_) : 0.0;
  }
  for (G4int k = 0; k < n; ++k) {
    const G4int j = k + shift;
    if (j >= 0 && j < n) shifted[j] = waveform[k];
  }
  return shifted;
}

G4int WaveformBuilder::SampleCount() const
{
  return kSampleCount;
}

G4double WaveformBuilder::SampleSpacing() const
{
  return kSampleSpacing;
}

G4double WaveformBuilder::TriggerTime() const
{
  return kTriggerTime;
}

G4double WaveformBuilder::StartTime() const
{
  return -TriggerTime();
}

}
