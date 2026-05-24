#ifndef WCSimStackingAction_H
#define WCSimStackingAction_H 1

#include "globals.hh"
#include "G4UserStackingAction.hh"
#include "WCSimDetectorConstruction.hh"

class G4Track;

class WCSimStackingAction : public G4UserStackingAction {

  public:
    WCSimStackingAction(WCSimDetectorConstruction*);
    virtual ~WCSimStackingAction();

  public:
    virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* aTrack);
    virtual void NewStage();
    virtual void PrepareNewEvent();
    static void ResetBGOScintillationOpticalPhotons();
    static void AddBGOEnergyDeposit(G4double edep);
    static G4int GetBGOScintillationOpticalPhotons();
    static G4double GetBGOEnergyDeposit();
    static G4int GetBGOStepsWithEnergyDeposit();

  private:
	  WCSimDetectorConstruction*   DetConstruct;
    static G4int bgoScintillationOpticalPhotons;
    static G4double bgoEnergyDeposit;
    static G4int bgoStepsWithEnergyDeposit;

};

#endif
