#ifndef WCSimAmBePMTResponse_h
#define WCSimAmBePMTResponse_h 1

#include <vector>

class TRandom3;

struct WCSimAmBePMTResponseResult
{
  WCSimAmBePMTResponseResult();

  int nScintillationAccepted;
  int nCherenkovAccepted;
  int nOtherAccepted;
  int triggered;
  double triggerTimeNs;
  double triggerGlobalTimeNs;
  std::vector<double> acceptedScintillationTimesNs;
  std::vector<double> acceptedCherenkovTimesNs;
  std::vector<double> waveform;
  std::vector<double> triggerAlignedWaveform;
};

class WCSimAmBePMTResponse
{
 public:
  static double WaveformBinWidthNs();
  static int WaveformBins();
  static double TriggerAlignmentTimeNs();

  static WCSimAmBePMTResponseResult Apply(
    const std::vector<double>& timesNs,
    const std::vector<double>& energiesEv,
    const std::vector<int>& processes,
    TRandom3& random);
};

#endif
