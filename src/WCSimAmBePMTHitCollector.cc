#include "WCSimAmBePMTHitCollector.hh"
#include "WCSimAmBePMTResponse.hh"

#include "G4ios.hh"
#include "TFile.h"
#include "TRandom3.h"
#include "TTree.h"

WCSimAmBePMTHitCollector::WCSimAmBePMTHitCollector()
  : fPhotonTree(0), fResponseTree(0), fRandom(new TRandom3(0)),
    fEvent(0), fProcess(0), fX(0.), fY(0.), fZ(0.), fTime(0.),
    fEnergy(0.), fCurrentEvent(-1), fResponseEvent(0),
    fNPhotonsEntering(0), fNScintillationAccepted(0),
    fNCherenkovAccepted(0), fNOtherAccepted(0), fTriggered(0),
    fTriggerTimeNs(-1.), fTriggerGlobalTimeNs(-1.),
    fWaveformBinWidthNs(WCSimAmBePMTResponse::WaveformBinWidthNs())
{}

WCSimAmBePMTHitCollector::~WCSimAmBePMTHitCollector()
{
  delete fRandom;
}

void WCSimAmBePMTHitCollector::StartOutputFile(TFile* outputFile)
{
  fPhotonTree = 0;
  fResponseTree = 0;
  fCurrentEvent = -1;
  if(!outputFile) return;

  outputFile->cd();
  fPhotonTree = new TTree("ph", "AmBe PMT optical-photon hits");
  fPhotonTree->Branch("evt", &fEvent, "evt/I");
  fPhotonTree->Branch("x", &fX, "x/D");
  fPhotonTree->Branch("y", &fY, "y/D");
  fPhotonTree->Branch("z", &fZ, "z/D");
  fPhotonTree->Branch("t", &fTime, "t/D");
  fPhotonTree->Branch("e", &fEnergy, "e/D");
  fPhotonTree->Branch("process", &fProcess, "process/I");
  fPhotonTree->Branch("process_name", &fProcessName);

  fResponseTree = new TTree("ambePMT", "AmBe source-tag PMT response");
  fResponseTree->Branch("evt", &fResponseEvent, "evt/I");
  fResponseTree->Branch("n_photons_entering", &fNPhotonsEntering,
                        "n_photons_entering/I");
  fResponseTree->Branch("n_scintillation_accepted", &fNScintillationAccepted,
                        "n_scintillation_accepted/I");
  fResponseTree->Branch("n_cherenkov_accepted", &fNCherenkovAccepted,
                        "n_cherenkov_accepted/I");
  fResponseTree->Branch("n_other_accepted", &fNOtherAccepted,
                        "n_other_accepted/I");
  fResponseTree->Branch("triggered", &fTriggered, "triggered/I");
  fResponseTree->Branch("trigger_time_ns", &fTriggerTimeNs,
                        "trigger_time_ns/D");
  fResponseTree->Branch("trigger_global_time_ns", &fTriggerGlobalTimeNs,
                        "trigger_global_time_ns/D");
  fResponseTree->Branch("waveform_bin_width_ns", &fWaveformBinWidthNs,
                        "waveform_bin_width_ns/D");
  fResponseTree->Branch("accepted_scintillation_times_ns",
                        &fAcceptedScintillationTimesNs);
  fResponseTree->Branch("accepted_cherenkov_times_ns",
                        &fAcceptedCherenkovTimesNs);
  fResponseTree->Branch("waveform", &fWaveform);
  fResponseTree->Branch("trigger_aligned_waveform", &fTriggerAlignedWaveform);
}

void WCSimAmBePMTHitCollector::BeginEvent(int event)
{
  fCurrentEvent = event;
  fPendingTimes.clear();
  fPendingEnergies.clear();
  fPendingProcesses.clear();
}

void WCSimAmBePMTHitCollector::RecordHit(int event, double xCm, double yCm,
                                         double zCm, double timeNs,
                                         double energyEv, int process,
                                         const std::string& processName)
{
  if(!fPhotonTree) return;

  fEvent = event;
  fX = xCm;
  fY = yCm;
  fZ = zCm;
  fTime = timeNs;
  fEnergy = energyEv;
  fProcess = process;
  fProcessName = processName;
  fPhotonTree->Fill();

  if(fCurrentEvent != event) BeginEvent(event);
  fPendingTimes.push_back(timeNs);
  fPendingEnergies.push_back(energyEv);
  fPendingProcesses.push_back(process);
}

void WCSimAmBePMTHitCollector::EndEvent(int event)
{
  if(!fResponseTree) return;
  if(fCurrentEvent != event) BeginEvent(event);

  const WCSimAmBePMTResponseResult response =
    WCSimAmBePMTResponse::Apply(fPendingTimes, fPendingEnergies,
                                fPendingProcesses, *fRandom);
  fResponseEvent = event;
  fNPhotonsEntering = static_cast<int>(fPendingTimes.size());
  fNScintillationAccepted = response.nScintillationAccepted;
  fNCherenkovAccepted = response.nCherenkovAccepted;
  fNOtherAccepted = response.nOtherAccepted;
  fTriggered = response.triggered;
  fTriggerTimeNs = response.triggerTimeNs;
  fTriggerGlobalTimeNs = response.triggerGlobalTimeNs;
  fAcceptedScintillationTimesNs = response.acceptedScintillationTimesNs;
  fAcceptedCherenkovTimesNs = response.acceptedCherenkovTimesNs;
  fWaveform = response.waveform;
  fTriggerAlignedWaveform = response.triggerAlignedWaveform;
  fResponseTree->Fill();
}

void WCSimAmBePMTHitCollector::Write()
{
  if(fPhotonTree){
    fPhotonTree->Write("", TObject::kOverwrite);
    G4cout << "Wrote AmBe PMT optical-photon hit tree with "
           << fPhotonTree->GetEntries() << " rows" << G4endl;
    fPhotonTree = 0;
  }

  if(fResponseTree){
    fResponseTree->Write("", TObject::kOverwrite);
    G4cout << "Wrote AmBe PMT response tree with "
           << fResponseTree->GetEntries() << " events" << G4endl;
    fResponseTree = 0;
  }
}
