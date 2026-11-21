#ifndef AmBeTaggerPmtResponse_h
#define AmBeTaggerPmtResponse_h 1


#include "globals.hh"

namespace AmBeTagger
{
class PmtResponse final
{
 public:
  G4double QuantumEfficiency(G4double photonEnergy) const;
  G4bool ProducesPhotoelectron(G4double photonEnergy) const;
};
}
#endif
