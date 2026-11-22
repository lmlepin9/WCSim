#ifndef WCSimRunAction_h
#define WCSimRunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"
#include "G4String.hh"

#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "WCSimRootEvent.hh"
#include "WCSimRootGeom.hh"
#include "WCSimRootOptions.hh"
#include "WCSimDetectorConstruction.hh"
#include "WCSimRandomParameters.hh"

class G4Run;
class WCSimRunActionMessenger;

class WCSimRunAction : public G4UserRunAction
{
public:
  WCSimRunAction(WCSimDetectorConstruction*, WCSimRandomParameters*);
  ~WCSimRunAction();

public:
  void BeginOfRunAction(const G4Run*);
  void EndOfRunAction(const G4Run*);
  void SetRootFileName(G4String fname) { RootFileName = fname; }
  void SetRootFileNameBase(G4String fname) { RootFileNameBase = fname; }
  G4String GetRootFileName() { return RootFileName; }
  G4String GetRootFileNameBase() { return RootFileNameBase; }
  void FillGeoTree();
  TTree* GetTree(){ return WCSimTree; }
  TBranch* GetBranch(G4String detectorElement){
    if(detectorElement=="tank") return wcsimrooteventbranch;
    else if(detectorElement=="mrd")  return wcsimrooteventbranch_mrd;
    else if(detectorElement=="facc") return wcsimrooteventbranch_facc;
    else G4cout<<"Unkown detector element"<<G4endl;}
  // AmBe tagger: per-event BGO branches (only created when the tagger is on)
  void SetBGOEventData(double edepMeV, double evisMeV, int nScint, int nCher, int nTracked){
    bgo_edep = edepMeV;
    bgo_evis = evisMeV;
    bgo_nscint_created = nScint;
    bgo_ncher_created = nCher;
    bgo_nphot_tracked = nTracked;
  }
  // AmBe tagger PMT: photoelectron count and waveform (amplitudes in mV,
  // times in ns of Geant4 global time, integral in mV*ns)
  void SetTaggerWaveformData(int nPE, double peakmV, double peakTimeNs, double integralmVns,
                             const std::vector<double>& waveformmV){
    tagger_npe = nPE;
    tagger_wf_peak = peakmV;
    tagger_wf_peak_time = peakTimeNs;
    tagger_wf_integral = integralmVns;
    tagger_wf = waveformmV;
  }
  // Must be called exactly once per event: wcsimT is filled branch by branch.
  void FillBGOBranches(){
    if(bgoedepbranch) bgoedepbranch->Fill();
    if(bgoevisbranch) bgoevisbranch->Fill();
    if(bgonscintbranch) bgonscintbranch->Fill();
    if(bgoncherbranch) bgoncherbranch->Fill();
    if(bgonphottrackedbranch) bgonphottrackedbranch->Fill();
    if(taggernpebranch) taggernpebranch->Fill();
    if(taggerwfpeakbranch) taggerwfpeakbranch->Fill();
    if(taggerwfpeaktimebranch) taggerwfpeaktimebranch->Fill();
    if(taggerwfintegralbranch) taggerwfintegralbranch->Fill();
    if(taggerwfbranch) taggerwfbranch->Fill();
  }
  TTree* GetGeoTree(){return geoTree;}
  TTree* GetOptionsTree(){return optionsTree;}
  WCSimRootGeom* GetRootGeom(){return wcsimrootgeom;}
  WCSimRootEvent* GetRootEvent(G4String detectorElement){
    if(detectorElement=="tank") return wcsimrootsuperevent;
    if(detectorElement=="mrd") return wcsimrootsuperevent_mrd;
    if(detectorElement=="facc") return wcsimrootsuperevent_facc;}

  void SetTree(TTree* tree){WCSimTree=tree;}
  void SetBranch(TBranch* branchin, G4String detectorElement){
    if(detectorElement=="tank") wcsimrooteventbranch=branchin;
    if(detectorElement=="mrd") wcsimrooteventbranch_mrd=branchin;
    if(detectorElement=="facc") wcsimrooteventbranch_facc=branchin;}
  void SetGeoTree(TTree* tree){geoTree=tree;}
  void SetRootEvent(WCSimRootEvent* revent, G4String detectorElement){
    if(detectorElement=="tank") wcsimrootsuperevent=revent;
    if(detectorElement=="mrd") wcsimrootsuperevent_mrd=revent;
    if(detectorElement=="facc") wcsimrootsuperevent_facc=revent;}
  void SetRootGeom(WCSimRootGeom* rgeom){wcsimrootgeom=rgeom;}
  WCSimRootOptions* GetRootOptions(){return wcsimrootoptions;}
  int  GetNumberOfEventsGenerated() { return numberOfEventsGenerated;}
  int  GetNtuples(){return ntuples;}
  G4int GetOutputFileNum(){return OutputFileNum;}

  void incrementEventsGenerated() { numberOfEventsGenerated++;}
  void incrementWaterTubeHits()   { numberOfTimesWaterTubeHit++;} 
  void incrementFVWaterTubeHits() { numberOfTimesFVWaterTubeHit++;} 
  void incrementCatcherHits()     { numberOfTimesCatcherHit++;}
  void SetNtuples(int ntup) {ntuples=ntup;}
  void CloseOutputFile();
  void CreateNewOutputFile();

private:
  // MFechner : set by the messenger
  std::string RootFileName;
  std::string RootFileNameBase;
  G4int OutputFileNum;
  //
  TTree* WCSimTree;
  TBranch* wcsimrooteventbranch;
  TBranch* wcsimrooteventbranch_mrd;
  TBranch* wcsimrooteventbranch_facc;
  TTree* geoTree;
  TTree* optionsTree;
  WCSimRootEvent* wcsimrootsuperevent;
  WCSimRootEvent* wcsimrootsuperevent_mrd;
  WCSimRootEvent* wcsimrootsuperevent_facc;
  WCSimRootGeom* wcsimrootgeom;
  WCSimRootOptions* wcsimrootoptions;
  WCSimDetectorConstruction* wcsimdetector;
  WCSimRandomParameters* wcsimrandomparameters;

  // AmBe stuff
  double bgo_edep;          // total BGO energy deposit of the event [MeV]
  double bgo_evis;          // Birks-quenched (visible) BGO energy deposit [MeV]
  int bgo_nscint_created;  // scintillation photons created in the BGO
  int bgo_ncher_created;    // Cherenkov photons created in the BGO
  int bgo_nphot_tracked;    // BGO-born photons that survived the stacking action
  // Branch handles, 0 when the tagger is off
  TBranch* bgoedepbranch;
  TBranch* bgoevisbranch;
  TBranch* bgonscintbranch;
  TBranch* bgoncherbranch;
  TBranch* bgonphottrackedbranch;
  // AmBe tagger PMT readout. Waveform: 1500 samples, 2 ns apart; sample k is
  // at global time -700 ns + 2k ns. Empty (and the scalars 0) for events
  // without photoelectrons.
  int tagger_npe;              // photoelectrons from BGO-born photons
  double tagger_wf_peak;       // waveform maximum [mV]
  double tagger_wf_peak_time;  // global time of the maximum sample [ns]
  double tagger_wf_integral;   // integral over the whole window [mV*ns]
  std::vector<double> tagger_wf;      // waveform samples [mV]
  std::vector<double>* tagger_wf_ptr; // ROOT branch address for tagger_wf
  TBranch* taggernpebranch;
  TBranch* taggerwfpeakbranch;
  TBranch* taggerwfpeaktimebranch;
  TBranch* taggerwfintegralbranch;
  TBranch* taggerwfbranch;

  int numberOfEventsGenerated;
  int numberOfTimesWaterTubeHit;
  int numberOfTimesFVWaterTubeHit;
  int numberOfTimesCatcherHit;

  WCSimRunActionMessenger* messenger;
  int ntuples;  // 1 for ntuples to be written
  G4bool isANNIE;
};

#endif
