#ifndef WCSimAmBePMTHitCollector_h
#define WCSimAmBePMTHitCollector_h 1

#include <string>
#include <vector>

class TFile;
class TTree;
class TRandom3;

class WCSimAmBePMTHitCollector
{
 public:
  WCSimAmBePMTHitCollector();
  ~WCSimAmBePMTHitCollector();

  void StartOutputFile(TFile* outputFile);
  void BeginEvent(int event);
  void RecordHit(int event, double xCm, double yCm, double zCm,
                 double timeNs, double energyEv, int process,
                 const std::string& processName);
  void EndEvent(int event);
  void Write();

 private:
  TTree* fPhotonTree;
  TTree* fResponseTree;
  TRandom3* fRandom;
  int fEvent;
  int fProcess;
  std::string fProcessName;
  double fX;
  double fY;
  double fZ;
  double fTime;
  double fEnergy;
  int fCurrentEvent;
  std::vector<double> fPendingTimes;
  std::vector<double> fPendingEnergies;
  std::vector<int> fPendingProcesses;

  int fResponseEvent;
  int fNPhotonsEntering;
  int fNScintillationAccepted;
  int fNCherenkovAccepted;
  int fNOtherAccepted;
  int fTriggered;
  double fTriggerTimeNs;
  double fTriggerGlobalTimeNs;
  double fWaveformBinWidthNs;
  std::vector<double> fAcceptedScintillationTimesNs;
  std::vector<double> fAcceptedCherenkovTimesNs;
  std::vector<double> fWaveform;
  std::vector<double> fTriggerAlignedWaveform;
};

#endif
