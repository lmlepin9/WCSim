#ifndef AmBeTaggerSinglePhotoelectronGain_h
#define AmBeTaggerSinglePhotoelectronGain_h 1


#include "globals.hh"

namespace AmBeTagger
{
class SinglePhotoelectronGain final
{
 public:
  // Single-PE peak height in volts (see AmBeTaggerWaveformBuilder.hh).
  // Sample() draws a Gaussian, redrawing non-positive values.
  explicit SinglePhotoelectronGain(G4double mean = 0.0058,
                                   G4double sigma = 0.0023);

  G4double Sample() const;
  G4double Mean() const;
  G4double Sigma() const;

 private:
  G4double mean_ = 1.0;
  G4double sigma_ = 0.0;
};
}

#endif
