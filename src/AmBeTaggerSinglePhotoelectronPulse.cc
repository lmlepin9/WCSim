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
}

G4double SinglePhotoelectronPulse::Shape(G4double pulseTime)
{
  return pulseTime < 0.0 ? 0.0 : UnnormalisedShape(pulseTime);
}

G4double SinglePhotoelectronPulse::TransitTime()
{
  return kTransitTime;
}
}  // namespace AmBeTagger
