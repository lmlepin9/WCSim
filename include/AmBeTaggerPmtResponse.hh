#ifndef AmBeTaggerPmtResponse_h
#define AmBeTaggerPmtResponse_h 1


#include "globals.hh"

namespace AmBeTagger
{
class PmtResponse final
{
 public:
  G4double QuantumEfficiency(G4double photonEnergy) const;
  // Peak of the QE curve. The curve is linearly interpolated, so this is an
  // upper bound of QuantumEfficiency() at every energy.
  G4double MaxQuantumEfficiency() const;
  // Full QE: photon was not culled at birth
  G4bool ProducesPhotoelectron(G4double photonEnergy) const;
  // QE(E)/QE_max: photon already survived the stacking-level cut at QE_max
  // (/WCSim/AmBe/TaggerStackingQE true), so the product is QE(E)
  G4bool ProducesPhotoelectronAfterCulling(G4double photonEnergy) const;
};
}
#endif
