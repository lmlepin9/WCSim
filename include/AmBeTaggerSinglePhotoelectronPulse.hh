#ifndef AmBeTaggerSinglePhotoelectronPulse_h
#define AmBeTaggerSinglePhotoelectronPulse_h 1


#include "globals.hh"

namespace AmBeTagger
{
class SinglePhotoelectronPulse final
{
 public:
  G4double Amplitude(G4double timeAfterPe) const;
  // Rise/fall shape before normalisation, t measured from the pulse start
  static G4double Shape(G4double pulseTime);
  G4double TransitTime() const;
};
}

#endif
