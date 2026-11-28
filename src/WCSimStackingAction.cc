#include "WCSimStackingAction.hh"
#include "WCSimDetectorConstruction.hh"
#include "WCSimTuningParameters.hh"

#include "G4Track.hh"
#include "G4TrackStatus.hh"
#include "G4VProcess.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"
#include "G4Navigator.hh"
#include "G4TransportationManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTypes.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include <iomanip>

//class WCSimDetectorConstruction;

WCSimStackingAction::WCSimStackingAction(WCSimDetectorConstruction* myDet):DetConstruct(myDet) {;}
WCSimStackingAction::~WCSimStackingAction(){;}


G4ClassificationOfNewTrack WCSimStackingAction::ClassifyNewTrack
(const G4Track* aTrack) 
{
  
  G4String WCIDCollectionName = DetConstruct->GetIDCollectionName();
  G4ClassificationOfNewTrack classification    = fWaiting;
  G4ParticleDefinition*      particleType      = aTrack->GetDefinition();

  //G4cout <<"WCIDCollectionName: "<<WCIDCollectionName<<G4endl;

  //return classification;        //bypass QE photon check
  
  // Make sure it is an optical photon
  if( particleType == G4OpticalPhoton::OpticalPhotonDefinition() ){
      // MF : translated from skdetsim : better to increase the number of photons
      // than to throw in a global factor at Digitization time !
      // XQ: get the maximum QE and multiply it by the ratio
      // only work for the range between 240 nm and 660 nm for now 
      // Even with WLS

      // Debug: primary photons (no creator process) skip QE culling if requested
      // via /WCSim/AmBe/BypassQEForPrimaryPhotons true
      if(aTrack->GetCreatorProcess()==nullptr && DetConstruct->GetBypassQEForPrimaryPhotons()){
        return classification;
      }

      // Optical photons born in the AmBe tagger BGO: the tank PMT QE does not
      // apply. With /WCSim/AmBe/TaggerStackingQE true they are kept with
      // probability QE_max of the tagger PMT, and WCSimSteppingAction accepts
      // them at the photocathode with QE(E)/QE_max; otherwise all are tracked.
      // Primaries have no touchable yet, so GetVolume() is null and they never
      // get here (the photocathode then applies the full QE to them).
      G4VPhysicalVolume* volume = aTrack->GetVolume();
      G4LogicalVolume* bgoVol = DetConstruct->GetBGOScoringVol();
      if(volume && bgoVol && volume->GetLogicalVolume() == bgoVol){
        if(DetConstruct->GetTaggerStackingQE() &&
           G4UniformRand() > TaggerPmtResponse.MaxQuantumEfficiency()){
          classification = fKill;
        }
        return classification;
      }


      G4float photonWavelength = (2.0*M_PI*197.3)/(aTrack->GetTotalEnergy()/CLHEP::eV);
      //G4float ratio = 1.; //1./(1.0-0.25); ??? increase the reported QE? Why???
      G4float ratio=1.;
      G4float wavelengthQE = 1.1;
      
      WCSimTuningParameters *tuning = (WCSimTuningParameters*) DetConstruct->Get_TuningParams();
      G4double QEratio = tuning->GetQERatio();
      ratio = QEratio;

      if(aTrack->GetCreatorProcess()==NULL) {
        // primary photons. I don't see why these should be treated differently... 
        if (DetConstruct->GetPMT_QE_Method()!=4){
          // primary photons use PMT_QE_Method Stacking_Only
          wavelengthQE  = DetConstruct->GetPMTQE(WCIDCollectionName,photonWavelength,1,240,660,ratio);
        } else {
          // ... unless using multiple PMT types, in which case this isn't supported
          // so use Stacking_And_SensitiveDetector instead
          wavelengthQE  = DetConstruct->GetPMTQE(WCIDCollectionName,photonWavelength,0,240,660,ratio);
        }
        
      } else if (((G4VProcess*)(aTrack->GetCreatorProcess()))->GetProcessType()!=3){
          // all normal photons here:
          
          if (DetConstruct->GetPMT_QE_Method()==1){
            // Stacking_Only
            wavelengthQE  = DetConstruct->GetPMTQE(WCIDCollectionName,photonWavelength,1,240,660,ratio);
          }else if (DetConstruct->GetPMT_QE_Method()==2||DetConstruct->GetPMT_QE_Method()==4){
            // Stacking_And_SensitiveDetector || Multi_Tank_Types
            wavelengthQE  = DetConstruct->GetPMTQE(WCIDCollectionName,photonWavelength,0,240,660,ratio);
          }else if (DetConstruct->GetPMT_QE_Method()==3){
            // SensitiveDetector_Only
            wavelengthQE = 1.1;
          } else {
            G4cerr<<"Uknown PMT_QE_Method: "<<DetConstruct->GetPMT_QE_Method()<<G4endl;
          }
      }
      // else {
      //   no culling for non-primary photons that have a Creator Process of type G4ProcessType[3] = fOptical
      //   There don't seem to be any such photons anyway.
      //}
      
      // prune the photon if desired
      if( G4UniformRand() > wavelengthQE ){ 
       classification = fKill; }
  }
  
  return classification;
}

void WCSimStackingAction::NewStage() {;}
void WCSimStackingAction::PrepareNewEvent() {;}

