#include "AmBeTaggerSinglePhotoelectronPulse.hh"

#include "G4SystemOfUnits.hh"

#include <cmath>

namespace AmBeTagger
{
namespace
{
const G4double kTauRise = 1.5 * ns;
const G4double kTauFall = 5.0 * ns;
const G4double kTransitTime = 26.0 * ns; // Now following PMT datasheet

G4double UnnormalisedShape(G4double pulseTime)
{
  return (1.0 - std::exp(-pulseTime / kTauRise))
      * std::exp(-pulseTime / kTauFall);
}

// The shape peaks at t* = tauRise * ln(1 + tauFall/tauRise); dividing by its
// value there makes the pulse peak 1, so the gain is the single-PE peak height.
const G4double kPeakNormalisation =
    1.0 / UnnormalisedShape(kTauRise * std::log(1.0 + kTauFall / kTauRise));
}

G4double SinglePhotoelectronPulse::Shape(G4double pulseTime)
{
  return pulseTime < 0.0 ? 0.0 : UnnormalisedShape(pulseTime);
}

G4double SinglePhotoelectronPulse::TransitTime() const
{
  return kTransitTime;
}

G4double SinglePhotoelectronPulse::Amplitude(G4double timeAfterPe) const
{
  const G4double pulseTime = timeAfterPe - kTransitTime;
  if (pulseTime < 0.0) {
    return 0.0;
  }

  return kPeakNormalisation * UnnormalisedShape(pulseTime);
}
}  // namespace AmBeTagger
