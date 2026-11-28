#ifndef AmBeTaggerSinglePhotoelectronPulse_h
#define AmBeTaggerSinglePhotoelectronPulse_h 1


#include "globals.hh"

namespace AmBeTagger
{
class SinglePhotoelectronPulse final
{
 public:
  // Unnormalised rise/fall shape, t measured from the pulse start
  static G4double Shape(G4double pulseTime);
  // Delay between the photoelectron and the pulse start
  static G4double TransitTime();
};
}

#endif
