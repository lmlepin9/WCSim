#ifndef AmBeTaggerGeometry_h
#define AmBeTaggerGeometry_h 1

#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"

class G4LogicalVolume;

// Builds the AmBe tagger (PVC housing, air cavity, BGO, PMT, source) inside a
// mother volume. Positions are in the tagger frame: BGO centre at z = 0, PMT
// towards +z, source towards -z.
class AmBeTaggerGeometry
{
public:
  // Places the tagger housing in mother at pos with active rotation rot
  // (both in the mother frame) and returns the housing logical volume.
  // rot is copied, so a temporary is fine.
  G4LogicalVolume* Build(G4LogicalVolume* mother,
                         const G4ThreeVector& pos = G4ThreeVector(),
                         const G4RotationMatrix& rot = G4RotationMatrix());

  // Valid after Build(); needed later for the sensitive detectors.
  G4LogicalVolume* GetScoringVolume() const { return scoringVolume_; }
  G4LogicalVolume* GetPmtPlaneVolume() const { return pmtPlaneVolume_; }
  // PMT parts around the photocathode, for light-path diagnostics.
  G4LogicalVolume* GetPmtWindowVolume() const { return pmtWindowVolume_; }
  G4LogicalVolume* GetPmtEnvelopeVolume() const { return pmtEnvelopeVolume_; }
  G4LogicalVolume* GetPmtVacuumVolume() const { return pmtVacuumVolume_; }

private:
  G4LogicalVolume* scoringVolume_ = nullptr;
  G4LogicalVolume* pmtPlaneVolume_ = nullptr;
  G4LogicalVolume* pmtWindowVolume_ = nullptr;
  G4LogicalVolume* pmtEnvelopeVolume_ = nullptr;
  G4LogicalVolume* pmtVacuumVolume_ = nullptr;
};

#endif
