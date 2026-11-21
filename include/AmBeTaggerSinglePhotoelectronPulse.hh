#ifndef AmBeTaggerSinglePhotoelectronPulse_h
#define AmBeTaggerSinglePhotoelectronPulse_h 1


#include "globals.hh"

namespace AmBeTagger
{
class SinglePhotoelectronPulse final
{
 public:
  G4double Amplitude(G4double timeAfterPe) const;
};
}

#endif
