#include "WCSimDetectorMessenger.hh"

#include "WCSimDetectorConstruction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"

WCSimDetectorMessenger::WCSimDetectorMessenger(WCSimDetectorConstruction* WCSimDet)
:WCSimDetector(WCSimDet)
{ 
  WCSimDir = new G4UIdirectory("/WCSim/");
  WCSimDir->SetGuidance("Commands to change the geometry of the simulation");


  PMTConfig = new G4UIcmdWithAString("/WCSim/WCgeom",this);
  PMTConfig->SetGuidance("Set the geometry configuration for the WC.");
  PMTConfig->SetGuidance("Available options are:\n"
			  "SuperK\n"
			  "SuperK_20inchPMT_20perCent\n"
			  "SuperK_20inchBandL_20perCent\n"
			  "SuperK_12inchBandL_15perCent\n"
			  "SuperK_20inchBandL_14perCent\n"
			  "Cylinder_60x74_20inchBandL_14perCent\n"
			  "Cylinder_60x74_20inchBandL_40perCent\n"
			  "Cylinder_12inchHPD_15perCent\n"
			  "Cylinder\n"
			  "HyperK\n"
			  "EggShapedHyperK\n"
			  "EggShapedHyperK_withHPD\n"
			  "ANNIEp1\n"
			  "ANNIEp2\n"
			  "ANNIEp2v2\n"
			  "ANNIEp2v3\n"
			  "ANNIEp2v4\n"
			  "ANNIEp2v5\n"
			  "ANNIEp2v6\n"
			  "ANNIEP2v7\n"
			  );
  PMTConfig->SetParameterName("PMTConfig", false);
  PMTConfig->SetCandidates("SuperK "
			  "SuperK_20inchPMT_20perCent "
			  "SuperK_20inchBandL_20perCent "
			  "SuperK_12inchBandL_15perCent "
			  "SuperK_20inchBandL_14perCent "
			  "Cylinder_60x74_20inchBandL_14perCent "
			  "Cylinder_60x74_20inchBandL_40perCent "
			  "Cylinder_12inchHPD_15perCent "
			  "Cylinder "
			  "HyperK "
			  "EggShapedHyperK "
			  "EggShapedHyperK_withHPD "
			  "ANNIEp1 "
			  "ANNIEp2 "
			  "ANNIEp2v2 "
			  "ANNIEp2v3 "
			  "ANNIEp2v4 "
			  "ANNIEp2v5 "
			  "ANNIEp2v6 "
			  "ANNIEp2v7 "
			  );
  PMTConfig->AvailableForStates(G4State_PreInit, G4State_Idle);

  WCVisChoice = new G4UIcmdWithAString("/WCSim/WCVisChoice",this);
  WCVisChoice->SetGuidance("Set the visualization style for the WC.");
  WCVisChoice->SetGuidance("Available options are:\n"
			  "OGLSX\n"
			  "RayTracer\n"
			  );
  WCVisChoice->SetParameterName("WCVisChoice", false);
  WCVisChoice->SetCandidates("OGLSX "
			  "RayTracer "
			  );
  WCVisChoice->AvailableForStates(G4State_PreInit, G4State_Idle);


  PMTSize = new G4UIcmdWithAString("/WCSim/WCPMTsize",this);
  PMTSize->SetGuidance("Set alternate PMT size for the WC (Must be entered after geometry details are set).");
  PMTSize->SetGuidance("Available options 20inch 10inch");
  PMTSize->SetParameterName("PMTSize", false);
  PMTSize->SetCandidates("20inch 10inch");
  PMTSize->AvailableForStates(G4State_PreInit, G4State_Idle);

  SavePi0 = new G4UIcmdWithAString("/WCSim/SavePi0", this);
  SavePi0->SetGuidance("true or false");
  SavePi0->SetParameterName("SavePi0",false);
  SavePi0->SetCandidates("true false");
  SavePi0->AvailableForStates(G4State_PreInit, G4State_Idle);

  SaveCapture = new G4UIcmdWithAString("/WCSim/SaveCapture", this);
  SaveCapture->SetGuidance("true or false");
  SaveCapture->SetParameterName("SaveCapture",false);
  SaveCapture->SetCandidates("true false");
  SaveCapture->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Options for AmBeTagger -------------------------------------------------------
  AmBeDir = new G4UIdirectory("/WCSim/AmBe/");
  AmBeDir->SetGuidance("AmBe source/tagger geometry commands");

  IncludeAmBeTagger = new G4UIcmdWithABool("/WCSim/AmBe/IncludeAmBeTagger",this);
  IncludeAmBeTagger->SetGuidance("true or false | Add the AmBe tagger to the tank. This option has to be set in: jobOptions.mac");
  IncludeAmBeTagger->SetParameterName("IncludeAmBeTagger",false);
  IncludeAmBeTagger->AvailableForStates(G4State_PreInit, G4State_Idle);

  taggerPosition = new G4UIcmdWith3VectorAndUnit("/WCSim/AmBe/TaggerPosition",this);
  taggerPosition->SetGuidance("Set tagger position (BGO centre) in the tank water (WCBarrel) frame. This option has to be set in: jobOptions.mac");
  taggerPosition->SetParameterName("x","y","z",false);
  taggerPosition->SetDefaultUnit("cm");
  taggerPosition->SetUnitCategory("Length");
  taggerPosition->AvailableForStates(G4State_PreInit, G4State_Idle);

  taggerRotation = new G4UIcmdWith3VectorAndUnit("/WCSim/AmBe/TaggerRotation",this);
  taggerRotation->SetGuidance("Set tagger rotation about the WCBarrel axes (X, then Y, then Z). This option has to be set in: jobOptions.mac");
  taggerRotation->SetParameterName("rotX", "rotY", "rotZ",false);
  taggerRotation->SetDefaultUnit("deg");
  taggerRotation->SetUnitCategory("Angle");
  taggerRotation->AvailableForStates(G4State_PreInit, G4State_Idle);

  bypassQEForPrimaryPhotons = new G4UIcmdWithABool("/WCSim/AmBe/BypassQEForPrimaryPhotons",this);
  bypassQEForPrimaryPhotons->SetGuidance("true or false | Debug: do not apply stacking-level QE culling to primary optical photons (e.g. photon gun aimed at the tagger PMT). Default: false");
  bypassQEForPrimaryPhotons->SetParameterName("BypassQEForPrimaryPhotons",true);
  bypassQEForPrimaryPhotons->SetDefaultValue(false);
  bypassQEForPrimaryPhotons->AvailableForStates(G4State_PreInit, G4State_Idle);

  taggerReferencePmt = new G4UIcmdWithABool("/WCSim/AmBe/ReferencePmt",this);
  taggerReferencePmt->SetGuidance("true or false | Test: replace the tagger PMT front end (coupling, window, envelope, photocathode plane) by the BGOg4Sim reference PMT, a r = 12.25 mm cylinder in contact with the BGO front face that counts photons on entry. This option has to be set in: jobOptions.mac. Default: false");
  taggerReferencePmt->SetParameterName("ReferencePmt",true);
  taggerReferencePmt->SetDefaultValue(false);
  taggerReferencePmt->AvailableForStates(G4State_PreInit, G4State_Idle);

  taggerReferenceWaveform = new G4UIcmdWithABool("/WCSim/AmBe/ReferenceWaveform",this);
  taggerReferenceWaveform->SetGuidance("true or false | Test: build the tagger PMT waveform as BGOg4Sim's photon_histogram_builder.py does: unit-area single-PE pulse with 5 mV area per PE, PE times binned in 2 ns samples with per-sample gain smearing Normal(N*5 mV, sqrt(N)*0.6 mV), and the first 10 mV crossing shifted to 700 ns. Default: false (R1398-04 model: 5.8 mV peak, 40% per-PE gain spread, global time axis)");
  taggerReferenceWaveform->SetParameterName("ReferenceWaveform",true);
  taggerReferenceWaveform->SetDefaultValue(false);
  taggerReferenceWaveform->AvailableForStates(G4State_PreInit, G4State_Idle);

  //-----------------------------------------------------------------
  PMTQEMethod = new G4UIcmdWithAString("/WCSim/PMTQEMethod", this);
  PMTQEMethod->SetGuidance("Set the PMT configuration.");
  PMTQEMethod->SetGuidance("Available options are:\n"
			     "Stacking_Only\n"
			     "Stacking_And_SensitiveDetector\n"
			     "SensitiveDetector_Only\n"
			     "Multi_Tank_Types\n");
  PMTQEMethod->SetParameterName("PMTQEMethod", false);
  PMTQEMethod->SetCandidates("Stacking_Only "
			     "Stacking_And_SensitiveDetector "
			     "SensitiveDetector_Only "
			     "Multi_Tank_Types ");
  PMTQEMethod->AvailableForStates(G4State_PreInit, G4State_Idle);
  //-------------
  LAPPDQEMethod = new G4UIcmdWithAString("/WCSim/LAPPDQEMethod", this);
  LAPPDQEMethod->SetGuidance("Set the LAPPD configuration.");
  LAPPDQEMethod->SetGuidance("Available options are:\n"
			     "Stacking_Only\n"
			     "Stacking_And_SensitiveDetector\n"
			     "SensitiveDetector_Only\n"
			     "Multi_Tank_Types\n");
  LAPPDQEMethod->SetParameterName("LAPPDQEMethod", false);
  LAPPDQEMethod->SetCandidates("Stacking_Only "
			       "Stacking_And_SensitiveDetector "
			       "SensitiveDetector_Only "
			       "Multi_Tank_Types ");
  LAPPDQEMethod->AvailableForStates(G4State_PreInit, G4State_Idle);
  //________

  PMTCollEff = new G4UIcmdWithAString("/WCSim/PMTCollEff", this);
  PMTCollEff->SetGuidance("Set the PMT configuration.");
  PMTCollEff->SetGuidance("Available options are:\n"
			  "on\n"
			  "off\n");
  PMTCollEff->SetParameterName("PMTCollEff", false);
  PMTCollEff->SetCandidates("on "
			    "off ");
  PMTCollEff->AvailableForStates(G4State_PreInit, G4State_Idle);

  waterTank_Length = new G4UIcmdWithADoubleAndUnit("/WCSim/EggShapedHyperK/waterTank_Length", this);
  waterTank_Length->SetGuidance("Set the Length of the egg-shaped HyperK detector (unit: mm cm m).");
  waterTank_Length->SetParameterName("waterTank_length", true);
  waterTank_Length->SetDefaultValue(49500.);
  waterTank_Length->SetUnitCategory("Length");
  waterTank_Length->SetDefaultUnit("mm");
  waterTank_Length->SetUnitCandidates("mm cm m");

    cylinderTank_Height = new G4UIcmdWithADoubleAndUnit("/WCSim/Cylinder/cylinderTank_Height", this);
    cylinderTank_Height->SetGuidance("Set the height of cylinder detector (unit: mm cm m).");
    cylinderTank_Height->SetParameterName("cylinderTank_Height", true);
    cylinderTank_Height->SetDefaultValue(14000.);
    cylinderTank_Height->SetUnitCategory("Length");
    cylinderTank_Height->SetDefaultUnit("mm");
    cylinderTank_Height->SetUnitCandidates("mm cm m");

	cylinderTank_Diameter = new G4UIcmdWithADoubleAndUnit("/WCSim/Cylinder/cylinderTank_Diameter", this);
	cylinderTank_Diameter->SetGuidance("Set the diameter of cylinder detector (unit: mm cm m).");
	cylinderTank_Diameter->SetParameterName("cylinderTank_Diameter", true);
	cylinderTank_Diameter->SetDefaultValue(8000.);
	cylinderTank_Diameter->SetUnitCategory("Length");
	cylinderTank_Diameter->SetDefaultUnit("mm");
	cylinderTank_Diameter->SetUnitCandidates("mm cm m");

	cylinderTank_Coverage = new G4UIcmdWithADouble("/WCSim/Cylinder/cylinderTank_Coverage", this);
	cylinderTank_Coverage->SetGuidance("Set the coverage of cylinder detector.");
	cylinderTank_Coverage->SetParameterName("cylinderTank_Coverage", true);
	cylinderTank_Coverage->SetDefaultValue(40.);

	cylinderTank_PMTType = new G4UIcmdWithAString("/WCSim/Cylinder/cylinderTank_PMTType", this);
	cylinderTank_PMTType->SetGuidance("Set the geometry configuration for the WC.");
	cylinderTank_PMTType->SetGuidance("Available options are:\n"
								   "PMT20inch\n"
								   "PMT8inch\n"
								   "PMT10inch\n"
								   "PMT10inchHQE\n"
								   "PMT12inchHQE\n"
								   "HPD20inchHQE\n"
								   "HPD12inchHQE\n"
								   "BoxandLine20inchHQE\n"
								   "BoxandLine12inchHQE"
	);
	cylinderTank_PMTType->SetParameterName("cylinderTank_PMTType", false);
	cylinderTank_PMTType->SetCandidates("PMT20inch "
						     	        "PMT8inch "
							            "PMT10inch "
							            "PMT10inchHQE "
							            "PMT12inchHQE "
							            "HPD20inchHQE "
							            "HPD12inchHQE "
							            "BoxandLine20inchHQE "
							            "BoxandLine12inchHQE"
	);

  WCConstruct = new G4UIcmdWithoutParameter("/WCSim/Construct", this);
  WCConstruct->SetGuidance("Update detector construction with new settings.");
}

WCSimDetectorMessenger::~WCSimDetectorMessenger()
{
  delete PMTConfig;
  delete SavePi0;
  delete SaveCapture;
  delete PMTQEMethod;
  delete LAPPDQEMethod;
  delete PMTCollEff;
  delete waterTank_Length;
  delete WCVisChoice;
  delete tubeCmd;
  delete IncludeAmBeTagger;
  delete taggerPosition;
  delete taggerRotation;
  delete bypassQEForPrimaryPhotons;
  delete taggerReferencePmt;
  delete taggerReferenceWaveform;
  delete AmBeDir;
  delete distortionCmd;
  delete WCSimDir;
  delete WCConstruct;
}

void WCSimDetectorMessenger::SetNewValue(G4UIcommand* command,G4String newValue)
{    
	if( command == PMTConfig ) {
		WCSimDetector->SetIsUpright(false);
		WCSimDetector->SetIsEggShapedHyperK(false);
		if ( newValue == "SuperK") {
		  WCSimDetector->SetSuperKGeometry();
		} else if ( newValue == "SuperK_20inchPMT_20perCent" ){
		  WCSimDetector->SuperK_20inchPMT_20perCent();
		} else if ( newValue == "SuperK_20inchBandL_20perCent" ){
		  WCSimDetector->SuperK_20inchBandL_20perCent();
		} else if ( newValue == "SuperK_12inchBandL_15perCent" ) {
		  WCSimDetector->SuperK_12inchBandL_15perCent();
		} else if ( newValue == "SuperK_20inchBandL_14perCent" ) {
		  WCSimDetector->SuperK_20inchBandL_14perCent();
		} else if ( newValue == "Cylinder_60x74_20inchBandL_14perCent" ) {
		  WCSimDetector->Cylinder_60x74_20inchBandL_14perCent();
		} else if ( newValue == "Cylinder_60x74_20inchBandL_40perCent" ) {
		  WCSimDetector->Cylinder_60x74_20inchBandL_40perCent();
		} else if ( newValue == "Cylinder_12inchHPD_15perCent" ){
		  WCSimDetector->Cylinder_12inchHPD_15perCent();
		} else if (newValue == "Cylinder") {
			WCSimDetector->SetIsCylinder(true);
			WCSimDetector->CylinderGeometry();
		} else if ( newValue == "HyperK" ){
		  WCSimDetector->SetHyperKGeometry();
		} else if ( newValue == "EggShapedHyperK") {
		  WCSimDetector->SetIsEggShapedHyperK(true);
		  WCSimDetector->SetEggShapedHyperKGeometry();
		} else if ( newValue == "EggShapedHyperK_withHPD") {
		  WCSimDetector->SetIsEggShapedHyperK(true);
		  WCSimDetector->SetEggShapedHyperKGeometry_withHPD();
		} else if (newValue == "ANNIEp1") {
		  WCSimDetector->SetANNIEPhase1Geometry();
		} else if (newValue == "ANNIEp2") {
		  WCSimDetector->SetANNIEPhase2Geometry();
		} else if (newValue == "ANNIEp2v2") {
		  WCSimDetector->SetANNIEPhase2Geometryv2();
		} else if (newValue == "ANNIEp2v3") {
		  WCSimDetector->SetANNIEPhase2Geometryv3();
		} else if (newValue == "ANNIEp2v4") {
		  WCSimDetector->SetANNIEPhase2Geometryv4();
		} else if (newValue == "ANNIEp2v5") {
		  WCSimDetector->SetANNIEPhase2Geometryv5();
		} else if (newValue == "ANNIEp2v6") {
		  WCSimDetector->SetANNIEPhase2Geometryv6();
		} else if (newValue == "ANNIEp2v7") {
		  WCSimDetector->SetANNIEPhase2Geometryv7();
		}
		else {
		  G4cout << "That geometry choice not defined!" << G4endl;
		}
	}
  
	if (command == SavePi0){
	  G4cout << "Set the flag for saving pi0 info " << newValue << G4endl;
	  if (newValue=="true"){
	    WCSimDetector->SavePi0Info(true);
	  }else if (newValue == "false"){
	    WCSimDetector->SavePi0Info(false);
	  }else{
	    
	  }
	}

	if (command == IncludeAmBeTagger){
		G4cout << "Include AmBe Tagger geometry: " << newValue << G4endl;
		WCSimDetector->SetIncludeAmBeTagger(IncludeAmBeTagger->GetNewBoolValue(newValue));
	}

	if (command == taggerPosition){
		WCSimDetector->SetTaggerPosition(taggerPosition->GetNew3VectorValue(newValue));
	}

	if (command==taggerRotation){
		WCSimDetector->SetTaggerRotation(taggerRotation->GetNew3VectorValue(newValue));
	}

	if (command == bypassQEForPrimaryPhotons){
		G4cout << "Bypass stacking QE for primary optical photons: " << newValue << G4endl;
		WCSimDetector->SetBypassQEForPrimaryPhotons(bypassQEForPrimaryPhotons->GetNewBoolValue(newValue));
	}

	if (command == taggerReferencePmt){
		G4cout << "AmBe tagger reference (BGOg4Sim) PMT model: " << newValue << G4endl;
		WCSimDetector->SetTaggerReferencePmt(taggerReferencePmt->GetNewBoolValue(newValue));
	}

	if (command == taggerReferenceWaveform){
		G4cout << "AmBe tagger reference (BGOg4Sim) waveform model: " << newValue << G4endl;
		WCSimDetector->SetTaggerReferenceWaveform(taggerReferenceWaveform->GetNewBoolValue(newValue));
	}

	if (command == SaveCapture){
	  G4cout << "Set the flag for saving neutron capture info " << newValue << G4endl;
	  if (newValue=="true"){
	    WCSimDetector->SaveCaptureInfo(true);
	  }else if (newValue == "false"){
	     WCSimDetector->SaveCaptureInfo(false);
	  }else{
	    
	  }
	}

	if (command == PMTQEMethod){
	  G4cout << "Set PMT QE Method " << newValue << " ";
	  if (newValue == "Stacking_Only"){
	    WCSimDetector->SetPMT_QE_Method(1);
	    G4cout << "1";
	  }else if (newValue == "Stacking_And_SensitiveDetector"){
	    WCSimDetector->SetPMT_QE_Method(2);
	    G4cout << "2";
	  }else if (newValue == "SensitiveDetector_Only"){
	    WCSimDetector->SetPMT_QE_Method(3);
	    G4cout << "3";
	  }else if (newValue == "Multi_Tank_Types"){
	    WCSimDetector->SetPMT_QE_Method(4);
	    G4cout << "4";
	  }else{
	    
	  }
	  G4cout << G4endl;
	}
	if (command == LAPPDQEMethod){
	  G4cout << "Set LAPPD QE Method " << newValue << " ";
	  if (newValue == "Stacking_Only"){
	    WCSimDetector->SetLAPPD_QE_Method(1);
	    G4cout << "1";
	  }else if (newValue == "Stacking_And_SensitiveDetector"){
	    WCSimDetector->SetLAPPD_QE_Method(2);
	    G4cout << "2";
	  }else if (newValue == "SensitiveDetector_Only"){
	    WCSimDetector->SetLAPPD_QE_Method(3);
	    G4cout << "3";
	  }else if (newValue == "Multi_Tank_Types"){
	    WCSimDetector->SetLAPPD_QE_Method(4);
	    G4cout << "4";
	  }else{
	    
	  }
	  G4cout << G4endl;
	}

	if (command == PMTCollEff){
	  G4cout << "Set PMT Collection Efficiency " << newValue << " ";
	  if (newValue == "on"){
	    WCSimDetector->SetPMT_Coll_Eff(1);
	    G4cout << "1";
	  }else if (newValue == "off"){
	    WCSimDetector->SetPMT_Coll_Eff(0);
	    G4cout << "0";
	  }
	  G4cout << G4endl;
	}
	
	if (command == WCVisChoice){
	  G4cout << "Set Vis Choice " << newValue << " ";
	  if (newValue == "OGLSX"){
	    WCSimDetector->SetVis_Choice("OGLSX");
	  }else if (newValue == "RayTracer"){
	    WCSimDetector->SetVis_Choice("RayTracer");
	  }
	  G4cout << G4endl;
	}

	if (command == waterTank_Length){
	  bool isEggShapedHyperK = WCSimDetector->GetIsEggShapedHyperK();
	  if(isEggShapedHyperK == true){
	    G4cout << "Set Partition Length in a cylinder " << newValue << " " << G4endl;
	    WCSimDetector->SetwaterTank_Length(waterTank_Length->GetNewDoubleValue(newValue));
	    WCSimDetector->SetWaterTubeLength(waterTank_Length->GetNewDoubleValue(newValue));
	    // WCSimDetector->SetIsEggShapedHyperK(true);
	  }
	  else {
	    G4cout << "Not egg-shaped HyperK Geometry. Detector length unchanged." << G4endl;
	  }
	}

	if (command == cylinderTank_Height) {
		bool isCylinder = WCSimDetector->GetIsCylinder();
		if (isCylinder) {
			G4cout << "Set length of cylinder " << newValue << " " << G4endl;
            WCSimDetector->SetCylinderHeight(cylinderTank_Height->GetNewDoubleValue(newValue));
		} else {
			G4cout << "Not Cylinder Geometry. Detector height unchanged." << G4endl;
		}
	}

	if (command == cylinderTank_Diameter) {
		bool isCylinder = WCSimDetector->GetIsCylinder();
		if (isCylinder) {
			G4cout << "Set diameter of cylinder " << newValue << " " << G4endl;
			WCSimDetector->SetCylinderDiameter(cylinderTank_Diameter->GetNewDoubleValue(newValue));
		} else {
			G4cout << "Not Cylinder Geometry. Detector diameter unchanged." << G4endl;
		}
	}

	if (command == cylinderTank_Coverage) {
		bool isCylinder = WCSimDetector->GetIsCylinder();
		if (isCylinder) {
			G4cout << "Set coverage of cylinder " << newValue << "% " << G4endl;
			WCSimDetector->SetCylinderCoverage(cylinderTank_Coverage->GetNewDoubleValue(newValue));
		} else {
			G4cout << "Not Cylinder Geometry. Detector coverage unchanged." << G4endl;
		}
	}

	if (command == cylinderTank_PMTType) {
		bool isCylinder = WCSimDetector->GetIsCylinder();
		if (isCylinder) {
			G4cout << "Set PMT type of cylinder " << newValue << " " << G4endl;
			WCSimDetector->SetCylinderPMTType(newValue);
		} else {
			G4cout << "Not Cylinder Geometry. Detector coverage unchanged." << G4endl;
		}
	}

	if(command == PMTSize) {
		G4cout << "SET PMT SIZE" << G4endl;
		if ( newValue == "20inch"){
//			WCSimDetector->Set20inchPMTs();
		}else if (newValue == "10inch"){
//			WCSimDetector->Set10inchPMTs();
		}else
			G4cout << "That PMT size is not defined!" << G4endl;	
	}

	if(command == WCConstruct) {
		bool isCylinder = WCSimDetector->GetIsCylinder();
		if (isCylinder) {
			WCSimDetector->UpdateCylinderGeometry();
		}
		WCSimDetector->UpdateGeometry();
	}

}
