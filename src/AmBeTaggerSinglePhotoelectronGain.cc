#include "AmBeTaggerSinglePhotoelectronGain.hh"

#include "Randomize.hh"

namespace AmBeTagger
{
SinglePhotoelectronGain::SinglePhotoelectronGain(G4double mean,
                                                 G4double sigma)
    : mean_(mean),
      sigma_(sigma)
{
}

G4double SinglePhotoelectronGain::Sample() const
{
  if (sigma_ <= 0.0) {
    return mean_;
  }

  // A PMT never gives a non-positive single-PE charge: redraw those
  // (about 0.6% of samples at sigma/mean = 0.4).
  G4double gain = 0.0;
  do {
    gain = G4RandGauss::shoot(mean_, sigma_);
  } while (gain <= 0.0);
  return gain;
}

G4double SinglePhotoelectronGain::Mean() const
{
  return mean_;
}

G4double SinglePhotoelectronGain::Sigma() const
{
  return sigma_;
}
}
