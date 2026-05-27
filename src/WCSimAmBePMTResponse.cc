#include "WCSimAmBePMTResponse.hh"

#include "TRandom3.h"

#include <algorithm>
#include <cmath>

namespace {
const int kNEntries = 106;
const double kBinWidthNs = 2.0;
const int kNBins = 1500;
const double kTimeMaxNs = kNBins * kBinWidthNs;
const double kTriggerThreshold = 0.01;
const double kTriggerTimeNs = 700.0;
const double kNoiseStd = 0.001;
const double kGainMean = 5.0 / 1000.0;
const double kGainSigma = 0.6 / 1000.0;

const double kEnergyEv[kNEntries] = {
  1.771, 1.784, 1.797, 1.810, 1.823, 1.837, 1.850, 1.864, 1.878, 1.893,
  1.907, 1.922, 1.937, 1.952, 1.968, 1.984, 2.000, 2.016, 2.032, 2.049,
  2.066, 2.084, 2.101, 2.119, 2.138, 2.156, 2.175, 2.194, 2.214, 2.234,
  2.254, 2.275, 2.296, 2.317, 2.339, 2.362, 2.384, 2.407, 2.431, 2.455,
  2.480, 2.505, 2.530, 2.556, 2.583, 2.610, 2.638, 2.666, 2.695, 2.725,
  2.755, 2.786, 2.818, 2.850, 2.883, 2.917, 2.952, 2.987, 3.024, 3.061,
  3.100, 3.139, 3.179, 3.220, 3.263, 3.306, 3.351, 3.397, 3.444, 3.492,
  3.542, 3.594, 3.646, 3.701, 3.757, 3.815, 3.874, 3.936, 3.999, 4.065,
  4.133, 4.203, 4.275, 4.350, 4.428, 4.508, 4.592, 4.678, 4.768, 4.862,
  4.959, 5.060, 5.166, 5.276, 5.390, 5.510, 5.635, 5.767, 5.904, 6.048,
  6.199, 6.358, 6.525, 6.702, 6.888, 7.085
};

const double kQePercent[kNEntries] = {
  0.000, 0.000, 0.000, 0.042, 0.057, 0.083, 0.116, 0.153, 0.197, 0.256,
  0.336, 0.441, 0.568, 0.716, 0.886, 1.075, 1.283, 1.512, 1.764, 2.040,
  2.338, 2.658, 3.001, 3.370, 3.770, 4.204, 4.676, 5.189, 5.741, 6.322,
  6.923, 7.533, 8.144, 8.763, 9.380, 9.997, 10.634, 11.302, 12.025, 12.699,
  13.401, 14.094, 14.800, 15.489, 16.190, 16.911, 17.656, 18.365, 19.010, 19.542,
  20.040, 20.541, 21.102, 21.692, 22.310, 22.883, 23.360, 23.673, 23.903, 24.082,
  24.276, 24.408, 24.363, 24.240, 24.074, 23.900, 23.718, 23.505, 23.170, 22.827,
  22.414, 21.974, 21.498, 20.970, 20.491, 20.076, 19.691, 19.317, 18.954, 18.584,
  18.180, 17.704, 17.156, 16.541, 15.937, 15.321, 14.699, 14.044, 13.355, 12.648,
  11.913, 11.153, 10.384, 9.636, 8.906, 8.192, 7.494, 6.788, 6.053, 5.289,
  4.577, 4.003, 3.408, 0.000, 0.000, 0.000
};

double QuantumEfficiency(double energyEv)
{
  if(energyEv <= kEnergyEv[0]) return kQePercent[0] / 100.;
  if(energyEv >= kEnergyEv[kNEntries - 1]) return kQePercent[kNEntries - 1] / 100.;

  const double* upper = std::upper_bound(kEnergyEv, kEnergyEv + kNEntries, energyEv);
  const int hi = static_cast<int>(upper - kEnergyEv);
  const int lo = hi - 1;
  const double fraction = (energyEv - kEnergyEv[lo]) /
                          (kEnergyEv[hi] - kEnergyEv[lo]);
  return ((1. - fraction) * kQePercent[lo] +
          fraction * kQePercent[hi]) / 100.;
}

std::vector<double> SinglePhotonResponse()
{
  std::vector<double> response;
  for(double time = -50.; time < 100.; time += kBinWidthNs){
    const double shifted = time - 23.;
    response.push_back(shifted < 0. ? 0. :
                       (1. - std::exp(-shifted / 1.5)) *
                       std::exp(-shifted / 5.));
  }

  double sum = 0.;
  for(size_t i = 0; i < response.size(); ++i) sum += response[i];
  if(sum > 0.){
    for(size_t i = 0; i < response.size(); ++i) response[i] /= sum;
  }
  return response;
}

std::vector<int> HistogramTimes(const std::vector<double>& times)
{
  std::vector<int> hist(kNBins, 0);
  for(size_t i = 0; i < times.size(); ++i){
    const double shiftedTime = times[i] + kTriggerTimeNs;
    if(shiftedTime < 0. || shiftedTime >= kTimeMaxNs) continue;
    const int bin = static_cast<int>(shiftedTime / kBinWidthNs);
    if(bin >= 0 && bin < kNBins) ++hist[bin];
  }
  return hist;
}
}

WCSimAmBePMTResponseResult::WCSimAmBePMTResponseResult()
  : nScintillationAccepted(0), nCherenkovAccepted(0), nOtherAccepted(0),
    triggered(0), triggerTimeNs(-1.), triggerGlobalTimeNs(-1.)
{}

double WCSimAmBePMTResponse::WaveformBinWidthNs()
{
  return kBinWidthNs;
}

int WCSimAmBePMTResponse::WaveformBins()
{
  return kNBins;
}

double WCSimAmBePMTResponse::TriggerAlignmentTimeNs()
{
  return kTriggerTimeNs;
}

WCSimAmBePMTResponseResult WCSimAmBePMTResponse::Apply(
  const std::vector<double>& timesNs,
  const std::vector<double>& energiesEv,
  const std::vector<int>& processes,
  TRandom3& random)
{
  WCSimAmBePMTResponseResult result;
  const size_t numberOfHits =
    std::min(timesNs.size(), std::min(energiesEv.size(), processes.size()));

  for(size_t hit = 0; hit < numberOfHits; ++hit){
    if(random.Rndm() > QuantumEfficiency(energiesEv[hit])) continue;

    if(processes[hit] == 0){
      ++result.nScintillationAccepted;
      result.acceptedScintillationTimesNs.push_back(timesNs[hit]);
    } else if(processes[hit] == 1){
      ++result.nCherenkovAccepted;
      result.acceptedCherenkovTimesNs.push_back(timesNs[hit]);
    } else {
      ++result.nOtherAccepted;
    }
  }

  const std::vector<int> histScintillation =
    HistogramTimes(result.acceptedScintillationTimesNs);
  const std::vector<int> histCherenkov =
    HistogramTimes(result.acceptedCherenkovTimesNs);
  const std::vector<double> singlePhotonResponse = SinglePhotonResponse();

  result.waveform.assign(kNBins, 0.);
  for(int bin = 0; bin < kNBins; ++bin){
    const int photons = histScintillation[bin] + histCherenkov[bin];
    if(!photons) continue;
    const double pulse =
      random.Gaus(photons * kGainMean,
                  std::sqrt(static_cast<double>(photons)) * kGainSigma);
    for(size_t responseBin = 0;
        responseBin < singlePhotonResponse.size(); ++responseBin){
      const int outputBin = bin + static_cast<int>(responseBin);
      if(outputBin >= kNBins) break;
      result.waveform[outputBin] += pulse * singlePhotonResponse[responseBin];
    }
  }
  for(int bin = 0; bin < kNBins; ++bin){
    result.waveform[bin] += random.Gaus(0., kNoiseStd);
  }

  const std::vector<double>::iterator threshold =
    std::find_if(result.waveform.begin(), result.waveform.end(),
                 [](double value){ return value > kTriggerThreshold; });
  if(threshold == result.waveform.end()) return result;

  result.triggered = 1;
  const int triggerBin = static_cast<int>(threshold - result.waveform.begin());
  result.triggerTimeNs = triggerBin * kBinWidthNs;
  result.triggerGlobalTimeNs = result.triggerTimeNs - kTriggerTimeNs;

  const int shift = static_cast<int>(kTriggerTimeNs / kBinWidthNs) -
                    triggerBin;
  result.triggerAlignedWaveform.assign(kNBins, 0.);
  for(int bin = 0; bin < kNBins; ++bin){
    result.triggerAlignedWaveform[bin] = random.Gaus(0., kNoiseStd);
  }
  for(int bin = 0; bin < kNBins; ++bin){
    const int outputBin = bin + shift;
    if(outputBin >= 0 && outputBin < kNBins){
      result.triggerAlignedWaveform[outputBin] = result.waveform[bin];
    }
  }
  return result;
}
