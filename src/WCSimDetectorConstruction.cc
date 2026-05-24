#include "WCSimDetectorConstruction.hh"
#include "WCSimDetectorMessenger.hh"
#include "WCSimTuningParameters.hh"

#include "G4Material.hh"
#include "G4Element.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"
#include "G4VisAttributes.hh"

#include "G4RunManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalSkinSurface.hh"
#include "WCSimDarkRateMessenger.hh"
#include "G4SolidStore.hh"
#include "G4GDMLParser.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4GeometryTolerance.hh"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
  G4bool HasMaterialProperty(G4MaterialPropertiesTable* mpt,
                             const G4String& propertyName)
  {
    return mpt && mpt->GetProperty(propertyName.c_str());
  }

  G4bool HasConstMaterialProperty(G4MaterialPropertiesTable* mpt,
                                  const G4String& propertyName)
  {
    return mpt && mpt->ConstPropertyExists(propertyName.c_str());
  }

  void CollectLogicalVolumesByName(G4LogicalVolume* logical,
                                   const G4String& nameFragment,
                                   std::vector<G4LogicalVolume*>& matches)
  {
    if(!logical) return;

    if(logical->GetName().find(nameFragment) != G4String::npos){
      matches.push_back(logical);
    }

    for(G4int i = 0; i < logical->GetNoDaughters(); ++i){
      G4VPhysicalVolume* daughter = logical->GetDaughter(i);
      if(daughter) CollectLogicalVolumesByName(daughter->GetLogicalVolume(),
                                               nameFragment,
                                               matches);
    }
  }
}

std::map<int, G4Transform3D> WCSimDetectorConstruction::tubeIDMap;
std::map<int, G4Transform3D> WCSimDetectorConstruction::mrdtubeIDMap;
std::map<int, G4Transform3D> WCSimDetectorConstruction::facctubeIDMap;
std::map<int, G4Transform3D> WCSimDetectorConstruction::lappdIDMap;
//std::map<int, cyl_location>  WCSimDetectorConstruction::tubeCylLocation;
hash_map<std::string, int, hash<std::string> > WCSimDetectorConstruction::tubeLocationMap;
hash_map<std::string, int, hash<std::string> > WCSimDetectorConstruction::mrdtubeLocationMap;
hash_map<std::string, int, hash<std::string> > WCSimDetectorConstruction::facctubeLocationMap;
hash_map<std::string, int, hash<std::string> > WCSimDetectorConstruction::lappdLocationMap;

WCSimDetectorConstruction::WCSimDetectorConstruction(G4int DetConfig,WCSimTuningParameters* WCSimTuningPars):WCSimTuningParams(WCSimTuningPars), noRot(0), rotatedmatx(0), upmtx(0), downmtx(0), rightmtx(0), leftmtx(0), scintSurface_op(0), MPTmylarSurface(0), lgSurface_op(0), lgsurf_MPT(0)
{

  // Decide if (only for the case of !1kT detector) should be upright or horizontal
  isUpright = false;
  isEggShapedHyperK  = false;
  constructtank = true;
  constructmrd = true;
  constructveto = true;
  worldExtentConfigured = false;

  debugMode = false;

  // --- AmBe housing ----------
  addAmBeHousing = false;
  amBeHousingGDMLPath = "AmBeHousing.gdml";
  amBeHousingCenter = G4ThreeVector(0.*cm, 0.*cm, 0.*cm);
  anniePMTTiltEnabled = true;
  anniePMTTiltAngle = -53.*deg;
  anniePMTTiltShift = 13.9*cm;

  myConfiguration = DetConfig;


  //-----------------------------------------------------
  // Create Materials
  //-----------------------------------------------------

  ConstructMaterials();

  //-----------------------------------------------------
  // Initialize things related to the tubeID
  //-----------------------------------------------------

  WCSimDetectorConstruction::tubeIDMap.clear();
  WCSimDetectorConstruction::mrdtubeIDMap.clear();
  WCSimDetectorConstruction::facctubeIDMap.clear();
  WCSimDetectorConstruction::lappdIDMap.clear();
  //WCSimDetectorConstruction::tubeCylLocation.clear();// (JF) Removed
  WCSimDetectorConstruction::tubeLocationMap.clear();
  WCSimDetectorConstruction::mrdtubeLocationMap.clear();
  WCSimDetectorConstruction::facctubeLocationMap.clear();
  WCSimDetectorConstruction::lappdLocationMap.clear();
  WCSimDetectorConstruction::PMTLogicalVolumes.clear();
  WCSimDetectorConstruction::LAPPDLogicalVolumes.clear();
  totalNumPMTs = 0;
  totalNumMrdPMTs = 0;
  totalNumFaccPMTs = 0;
  totalNumLAPPDs = 0;
  WCPMTExposeHeight= 0.;
  WCLAPPDExposeHeight= 0.;
  //-----------------------------------------------------
  // Set the default WC geometry.  This can be changed later.
  //-----------------------------------------------------

  //SetSuperKGeometry();
  //SetHyperKGeometry();
  //SetANNIEPhase1Geometry();
  //SetANNIEPhase2Geometry();
  //SetANNIEPhase2Geometryv2();
  //SetANNIEPhase2Geometryv3();
  //SetANNIEPhase2Geometryv4();
  //SetANNIEPhase2Geometryv5();
  //SetANNIEPhase2Geometryv6();
  SetANNIEPhase2Geometryv7();

  //-----------------------------------------------------
  // Set whether or not Pi0-specific info is saved
  //-----------------------------------------------------

  SavePi0Info(false);

  //-----------------------------------------------------
  // Set whether or not neutron capture info is saved
  //-----------------------------------------------------

  SaveCaptureInfo(true);

  //-----------------------------------------------------
  // Set the default method for implementing the PMT QE
  //-----------------------------------------------------
  SetPMT_QE_Method(1);

  //default is to use collection efficiency
  SetPMT_Coll_Eff(1);
  SetLAPPD_QE_Method(1);
  SetLAPPD_Coll_Eff(1);
  // set default visualizer to OGLSX
  SetVis_Choice("OGLSX");

  //-----------------------------------------------------
  // Make the detector messenger to allow changing geometry
  //-----------------------------------------------------

  messenger = new WCSimDetectorMessenger(this);
}

void WCSimDetectorConstruction::ApplyAmBeOpticalProperties(G4LogicalVolume* ambeHousingLog)
{
  const G4int nEntries = 13;
  G4double energy[nEntries] = {
    1.771*eV, 1.922*eV, 2.084*eV, 2.275*eV, 2.505*eV,
    2.755*eV, 2.987*eV, 3.220*eV, 3.492*eV, 3.874*eV,
    4.350*eV, 5.060*eV, 6.199*eV
  };

  G4double bgoScintillation[nEntries] = {
    0.00, 0.08, 0.28, 0.62, 0.95, 0.87, 0.42,
    0.08, 0.00, 0.00, 0.00, 0.00, 0.00
  };
  G4double bgoRIndex[nEntries] = {
    2.084, 2.095, 2.107, 2.124, 2.146, 2.174, 2.216,
    2.241, 2.289, 2.387, 2.473, 2.589, 2.718
  };
  G4double bgoAbsLength[nEntries] = {
    10.0*cm, 9.9*cm, 9.7*cm, 9.6*cm, 9.4*cm, 9.1*cm, 8.6*cm,
    8.0*cm, 7.3*cm, 2.6*cm, 1.0e-6*cm, 1.0e-6*cm, 1.0e-6*cm
  };

  G4Material* bgo = G4Material::GetMaterial("BGO", false);
  G4MaterialPropertiesTable* parsedBgoMPT =
    bgo ? bgo->GetMaterialPropertiesTable() : 0;
  if(!bgo){
    G4cerr << "WCSimDetectorConstruction::ApplyAmBeOpticalProperties(): "
           << "BGO material from " << amBeHousingGDMLPath << " was not found"
           << G4endl;
  } else if(!HasMaterialProperty(parsedBgoMPT, "SCINTILLATIONCOMPONENT1") ||
            !HasMaterialProperty(parsedBgoMPT, "RINDEX") ||
            !HasMaterialProperty(parsedBgoMPT, "ABSLENGTH")){
    G4MaterialPropertiesTable* bgoMPT = new G4MaterialPropertiesTable();
    bgoMPT->AddProperty("SCINTILLATIONCOMPONENT1", energy, bgoScintillation, nEntries);
    bgoMPT->AddProperty("SCINTILLATIONCOMPONENT2", energy, bgoScintillation, nEntries);
    bgoMPT->AddProperty("RINDEX", energy, bgoRIndex, nEntries);
    bgoMPT->AddProperty("ABSLENGTH", energy, bgoAbsLength, nEntries);
    bgoMPT->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV);
    bgoMPT->AddConstProperty("RESOLUTIONSCALE", 2.0);
    bgoMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 1.0*ns);
    bgoMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT2", 300.0*ns);
    bgoMPT->AddConstProperty("SCINTILLATIONYIELD1", 0.0);
    bgoMPT->AddConstProperty("SCINTILLATIONYIELD2", 1.0);
    bgo->SetMaterialPropertiesTable(bgoMPT);
  }
  if(bgo){
    G4MaterialPropertiesTable* bgoMPT = bgo->GetMaterialPropertiesTable();
    if(bgoMPT){
      // Geant4 10.7 defaults to the legacy scintillation timing mode, which
      // requires FASTCOMPONENT even when GDML provides SCINTILLATIONCOMPONENT1.
      if(!HasMaterialProperty(bgoMPT, "FASTCOMPONENT")){
        bgoMPT->AddProperty("FASTCOMPONENT", energy, bgoScintillation, nEntries);
      }
      if(!HasConstMaterialProperty(bgoMPT, "FASTTIMECONSTANT")){
        bgoMPT->AddConstProperty("FASTTIMECONSTANT", 300.0*ns);
      }
      if(!HasConstMaterialProperty(bgoMPT, "SCINTILLATIONYIELD")){
        bgoMPT->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV);
      }
      if(!HasConstMaterialProperty(bgoMPT, "RESOLUTIONSCALE")){
        bgoMPT->AddConstProperty("RESOLUTIONSCALE", 2.0);
      }
    }
  }

  G4double teflonRIndex[nEntries];
  G4double teflonReflectivity[nEntries];
  G4double teflonEfficiency[nEntries];
  for(G4int i = 0; i < nEntries; ++i){
    teflonRIndex[i] = 1.35;
    teflonReflectivity[i] = 0.90;
    teflonEfficiency[i] = 0.0;
  }

  const G4int nAbsEntries = 2;
  G4double teflonAbsEnergy[nAbsEntries] = {1.771*eV, 6.199*eV};
  G4double teflonAbsLength[nAbsEntries] = {1.0e-6*m, 1.0e-6*m};

  G4Material* teflon = G4Material::GetMaterial("Teflon", false);
  G4MaterialPropertiesTable* parsedTeflonMPT =
    teflon ? teflon->GetMaterialPropertiesTable() : 0;
  if(!teflon){
    G4cerr << "WCSimDetectorConstruction::ApplyAmBeOpticalProperties(): "
           << "Teflon material from " << amBeHousingGDMLPath << " was not found"
           << G4endl;
  } else if(!HasMaterialProperty(parsedTeflonMPT, "RINDEX") ||
            !HasMaterialProperty(parsedTeflonMPT, "ABSLENGTH") ||
            !HasMaterialProperty(parsedTeflonMPT, "REFLECTIVITY")){
    G4MaterialPropertiesTable* teflonMPT = new G4MaterialPropertiesTable();
    teflonMPT->AddProperty("RINDEX", energy, teflonRIndex, nEntries);
    teflonMPT->AddProperty("ABSLENGTH", teflonAbsEnergy, teflonAbsLength, nAbsEntries);
    teflonMPT->AddProperty("REFLECTIVITY", energy, teflonReflectivity, nEntries);
    teflonMPT->AddProperty("EFFICIENCY", energy, teflonEfficiency, nEntries);
    teflon->SetMaterialPropertiesTable(teflonMPT);
  }

  std::vector<G4LogicalVolume*> teflonWrappingLogs;
  CollectLogicalVolumesByName(ambeHousingLog,
                              "BGO_teflon_wrapping",
                              teflonWrappingLogs);
  G4OpticalSurface* teflonSurface = 0;
  for(size_t i = 0; i < teflonWrappingLogs.size(); ++i){
    if(!G4LogicalSkinSurface::GetSurface(teflonWrappingLogs[i])){
      if(!teflonSurface){
        G4MaterialPropertiesTable* teflonSurfaceMPT = new G4MaterialPropertiesTable();
        teflonSurfaceMPT->AddProperty("REFLECTIVITY", energy, teflonReflectivity, nEntries);
        teflonSurfaceMPT->AddProperty("EFFICIENCY", energy, teflonEfficiency, nEntries);

        teflonSurface = new G4OpticalSurface("BGO_Teflon_OpticalSurface_WCSim");
        teflonSurface->SetType(dielectric_metal);
        teflonSurface->SetModel(glisur);
        teflonSurface->SetFinish(ground);
        teflonSurface->SetPolish(0.9);
        teflonSurface->SetMaterialPropertiesTable(teflonSurfaceMPT);
      }
      new G4LogicalSkinSurface("BGO_Teflon_SkinSurface_WCSim",
                               teflonWrappingLogs[i],
                               teflonSurface);
    }
  }

  G4cout << "Applied AmBe BGO scintillation properties and teflon optical "
         << "behavior from " << amBeHousingGDMLPath << " to "
         << teflonWrappingLogs.size() << " teflon wrapping logical volume(s)"
         << G4endl;
}

void WCSimDetectorConstruction::PlaceAmBeHousing(G4LogicalVolume* motherLog)
{
  if(!addAmBeHousing || !motherLog) return;

  G4GDMLParser parser;
  parser.SetOverlapCheck(true);
  parser.Read(amBeHousingGDMLPath, false);

  G4VPhysicalVolume* gdmlWorldPhys = parser.GetWorldVolume();
  if(!gdmlWorldPhys){
    G4cerr << "WCSimDetectorConstruction::PlaceAmBeHousing(): failed to read "
           << amBeHousingGDMLPath << G4endl;
    return;
  }

  G4LogicalVolume* gdmlWorldLog = gdmlWorldPhys->GetLogicalVolume();
  G4LogicalVolume* ambeHousingLog = 0;
  G4VPhysicalVolume* barrelAirPhys = 0;
  for(G4int i = 0; i < gdmlWorldLog->GetNoDaughters(); ++i){
    G4VPhysicalVolume* daughter = gdmlWorldLog->GetDaughter(i);
    if(daughter && daughter->GetName() == "AmBeHousing"){
      ambeHousingLog = daughter->GetLogicalVolume();
      break;
    }
  }

  if(!ambeHousingLog){
    G4cerr << "WCSimDetectorConstruction::PlaceAmBeHousing(): could not find "
           << "the AmBeHousing physical volume in " << amBeHousingGDMLPath
           << G4endl;
    return;
  }

  for(G4int i = 0; i < ambeHousingLog->GetNoDaughters(); ++i){
    G4VPhysicalVolume* daughter = ambeHousingLog->GetDaughter(i);
    if(daughter && daughter->GetName().contains("barrel_air_vol")){
      barrelAirPhys = daughter;
      break;
    }
  }

  if(!barrelAirPhys){
    G4cerr << "WCSimDetectorConstruction::PlaceAmBeHousing(): could not find "
           << "barrel_air_vol inside AmBeHousing in " << amBeHousingGDMLPath
           << G4endl;
    return;
  }

  G4LogicalVolume* barrelAirLog = barrelAirPhys->GetLogicalVolume();
  G4ThreeVector barrelAirPosition = barrelAirPhys->GetObjectTranslation();
  ambeHousingLog->RemoveDaughter(barrelAirPhys);

  G4Tubs* ambeEnvelopeSolid =
    new G4Tubs("AmBeHousingEnvelope",
               0.*mm,
               45.*mm,
               240.*mm,
               0.*deg,
               360.*deg);
  G4LogicalVolume* ambeEnvelopeLog =
    new G4LogicalVolume(ambeEnvelopeSolid,
                        motherLog->GetMaterial(),
                        "AmBeHousingEnvelope",
                        0,
                        0,
                        0);

  G4VisAttributes* ambeVisAtt = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
  ambeVisAtt->SetForceSolid(true);
  ambeHousingLog->SetVisAttributes(ambeVisAtt);
  ambeEnvelopeLog->SetVisAttributes(G4VisAttributes::Invisible);

  new G4PVPlacement(0,
                    G4ThreeVector(),
                    ambeHousingLog,
                    "AmBeHousingShell",
                    ambeEnvelopeLog,
                    false,
                    0,
                    true);

  new G4PVPlacement(0,
                    barrelAirPosition,
                    barrelAirLog,
                    "barrel_air_vol_PV",
                    ambeEnvelopeLog,
                    false,
                    0,
                    true);

  ApplyAmBeOpticalProperties(ambeEnvelopeLog);

  new G4PVPlacement(0,
                    amBeHousingCenter,
                    ambeEnvelopeLog,
                    "AmBeHousing",
                    motherLog,
                    false,
                    0,
                    true);

  G4cout << "[DEBUG] Placed AmBe housing from " << amBeHousingGDMLPath
         << " at tank coordinates " << amBeHousingCenter/cm
         << " cm with a navigable envelope around the native GDML geometry"
         << G4endl;
}

void WCSimDetectorConstruction::SetANNIEDetectorComponents(G4String componentList)
{
  std::string requested = componentList;
  std::replace(requested.begin(), requested.end(), ',', ' ');
  std::replace(requested.begin(), requested.end(), ';', ' ');
  std::replace(requested.begin(), requested.end(), '+', ' ');
  std::transform(requested.begin(), requested.end(), requested.begin(),
                 [](unsigned char c){ return std::tolower(c); });

  std::istringstream tokens(requested);
  std::string token;
  G4bool newConstructTank = false;
  G4bool newConstructMRD = false;
  G4bool newConstructVeto = false;
  G4bool sawToken = false;

  while(tokens >> token){
    sawToken = true;
    if(token == "all"){
      newConstructTank = true;
      newConstructMRD = true;
      newConstructVeto = true;
    } else if(token == "none"){
      newConstructTank = false;
      newConstructMRD = false;
      newConstructVeto = false;
    } else if(token == "tank" || token == "annie" || token == "wc"){
      newConstructTank = true;
    } else if(token == "mrd"){
      newConstructMRD = true;
    } else if(token == "fmv" || token == "facc" || token == "veto"){
      newConstructVeto = true;
    } else {
      G4cerr << "Unknown ANNIE detector component '" << token
             << "'. Valid components are all, tank, mrd, and fmv/facc/veto. "
             << "Keeping previous component selection." << G4endl;
      return;
    }
  }

  if(!sawToken){
    G4cerr << "Empty ANNIE detector component list. Keeping previous component selection." << G4endl;
    return;
  }

  constructtank = newConstructTank;
  constructmrd = newConstructMRD;
  constructveto = newConstructVeto;

  G4cout << "ANNIE detector components enabled: tank="
         << (constructtank ? "true" : "false")
         << " mrd=" << (constructmrd ? "true" : "false")
         << " fmv=" << (constructveto ? "true" : "false")
         << G4endl;
}

#include "G4GeometryManager.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"

void WCSimDetectorConstruction::UpdateGeometry()
{


  G4bool geomChanged = true;
  G4RunManager::GetRunManager()->DefineWorldVolume(Construct(), geomChanged);

 }



WCSimDetectorConstruction::~WCSimDetectorConstruction(){
  for (unsigned int i=0;i<fpmts.size();i++){
    delete fpmts.at(i);
  }
  fpmts.clear();
  for (unsigned int i=0;i<fmrdpmts.size();i++){
    delete fmrdpmts.at(i);
  }
  fmrdpmts.clear();
    for (unsigned int i=0;i<ffaccpmts.size();i++){
    delete ffaccpmts.at(i);
  }
  ffaccpmts.clear();
  for (unsigned int i=0;i<flappds.size();i++){
    delete flappds.at(i);
  }
  flappds.clear();

  // MRD objects...
  // rotation matrices
  if(noRot) delete noRot;
  if(rotatedmatx) delete rotatedmatx;
  if(upmtx) delete upmtx;
  if(downmtx) delete downmtx;
  if(rightmtx) delete rightmtx;
  if(leftmtx) delete leftmtx;

  // optical surfaces and materials properties tables
  if(scintSurface_op) delete scintSurface_op;
  if(MPTmylarSurface) delete MPTmylarSurface;
  if(lgSurface_op) delete lgSurface_op;
  if(lgsurf_MPT) delete lgsurf_MPT;

  // logical border surfaces
  for(auto surface : bordersurfaces){
    delete surface;
  }
  bordersurfaces.clear();

  // visualisation attributes
    for(auto visatt : mrdvisattributes){
    delete visatt;
  }
  mrdvisattributes.clear();

}

G4VPhysicalVolume* WCSimDetectorConstruction::Construct()
{
  G4GeometryManager::GetInstance()->OpenGeometry();
  // having issues with StepTooSmall not being set - step sizes are ~1e-9, which *is* larger
  // than the kCarTolerance/2 (5e-10m), but StepTooSmall isn't set for some reason...
  // Try calling SetWorldMaximumExtent to have it calculated from World size.
  // This value must be setBEFORE ANY GEOMETRY is instantiated.
  // Seems to work - kCarTolerance is now much smaller, no more errors.
  if(isANNIE && !worldExtentConfigured){
    // let's try setting the tolerance to try to get rid of geometry errors
    // this takes the world extent - set in WCSimDetectorConstruction as expHallLength
    G4GeometryManager::GetInstance()->SetWorldMaximumExtent(3.*WCLength);
    worldExtentConfigured = true;
    G4cout << "Computed tolerance = "
           << G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/mm << " mm" << G4endl;
  }


  //--------------- AmBe housing debug -------------
  if(addAmBeHousing){
    G4cout << "[DEBUG]---------- AmBe housing construction enabled !!!--------------- " << G4endl;
  }

  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();
  G4LogicalBorderSurface::CleanSurfaceTable();
  G4LogicalSkinSurface::CleanSurfaceTable();
  WCSimDetectorConstruction::PMTLogicalVolumes.clear();
  WCSimDetectorConstruction::LAPPDLogicalVolumes.clear();

  totalNumPMTs = 0;
  totalNumMrdPMTs = 0;
  totalNumFaccPMTs = 0;
  totalNumLAPPDs = 0;

  //-----------------------------------------------------
  // Create Logical Volumes
  //-----------------------------------------------------

  // First create the logical volumes of the sub detectors.  After they are
  // created their size will be used to make the world volume.
  // Note the order is important because they rearrange themselves depending
  // on their size and detector ordering.

  G4LogicalVolume* logicWCBox;
  // Select between egg-shaped HyperK and cylinder
  if (isEggShapedHyperK) logicWCBox = ConstructEggShapedHyperK();
  else if (isANNIE) logicWCBox = ConstructANNIE(); // returns a 5x5x5m box MatryoshkaMother
  else logicWCBox = ConstructCylinder();
  G4cout << " WCLength       = " << WCLength/CLHEP::m << " m"<< G4endl;

  //-------------------------------

  // Now make the detector Hall.  The lengths of the subdectors
  // were set above.

  G4double expHallLength = 3.*WCLength; //jl145 - extra space to simulate cosmic muons more easily

  G4cout << " expHallLength = " << expHallLength / CLHEP::m << G4endl;
  G4double expHallHalfLength = 0.5*expHallLength;

  G4Box* solidExpHall = new G4Box("expHall",
				  expHallHalfLength,
				  expHallHalfLength,
				  expHallHalfLength);

  G4LogicalVolume* logicExpHall =
    new G4LogicalVolume(solidExpHall,
			G4Material::GetMaterial("Vacuum"),
			"expHall",
			0,0,0);

  // Now set the visualization attributes of the logical volumes.

  //   logicWCBox->SetVisAttributes(G4VisAttributes::Invisible);
  logicExpHall->SetVisAttributes(G4VisAttributes::Invisible);

  //-----------------------------------------------------
  // Create and place the physical Volumes
  //-----------------------------------------------------
  // Experimental Hall
  G4VPhysicalVolume* physiExpHall =
    new G4PVPlacement(0,G4ThreeVector(),
  		      logicExpHall,
  		      "expHall",
  		      0,false,0,true);

  // Water Cherenkov Detector (WC) mother volume
  // WC Box, nice to turn on for x and y views to provide a frame:

	  //G4RotationMatrix* rotationMatrix = new G4RotationMatrix;
	  //rotationMatrix->rotateX(90.*deg);
	  //rotationMatrix->rotateZ(90.*deg);

  G4ThreeVector genPosition = G4ThreeVector(0., 0., WCPosition);
  G4VPhysicalVolume* physiWCBox =
    new G4PVPlacement(0,
		      genPosition,
		      logicWCBox,
		      "WCBox",
		      logicExpHall,
		      false,
		      0);

  // Reset the tubeID and tubeLocation maps before refiling them
  tubeIDMap.clear();
  mrdtubeIDMap.clear();
  facctubeIDMap.clear();
  lappdIDMap.clear();
  tubeLocationMap.clear();
  mrdtubeLocationMap.clear();
  facctubeLocationMap.clear();
  lappdLocationMap.clear();


  // Traverse and print the geometry Tree

  //  TraverseReplicas(physiWCBox, 0, G4Transform3D(),
  //	   &WCSimDetectorConstruction::PrintGeometryTree) ;

  TraverseReplicas(physiWCBox, 0, G4Transform3D(),
	           &WCSimDetectorConstruction::DescribeAndRegisterPMT) ;

  TraverseReplicas(physiWCBox, 0, G4Transform3D(),
		   &WCSimDetectorConstruction::GetWCGeom) ;
  DumpGeometryTableToFile();

  for(auto apmt : WCTubeCollectionMap){
    int tubeid = apmt.first;
    G4String collectionname = apmt.second;
    if(TubeIdsByCollection.count(collectionname)==0){
      TubeIdsByCollection.emplace(collectionname,std::vector<int>{tubeid});
    } else {
      TubeIdsByCollection.at(collectionname).push_back(tubeid);
    }
  }

  //G4cout<<"Writing GDML output file"<<G4endl;
  //G4String GDMLOutFilename = "anniegeomv3.gdml";
  //G4GDMLParser parser;  // Write GDML file
  //parser.Write(GDMLOutFilename, logicExpHall);
  //G4cout<<"GDML file "<<GDMLOutFilename<<" written"<<G4endl;

  // Return the pointer to the physical experimental hall
  return physiExpHall;


}

WCSimLAPPDObject *WCSimDetectorConstruction::CreateLAPPDObject(G4String LAPPDType, G4String CollectionName2)
{
  if (LAPPDType == "lappd"){
     WCSimLAPPDObject* lappd = new LAPPD;
     WCSimDetectorConstruction::SetLAPPDPointer(lappd, CollectionName2);
      return lappd;
  }
}

WCSimPMTObject *WCSimDetectorConstruction::CreatePMTObject(G4String PMTType, G4String CollectionName)
{
  if (PMTType == "PMT20inch"){
     WCSimPMTObject* PMT = new PMT20inch;
     WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
      return PMT;
  }
  else if (PMTType == "PMT8inch"){
    WCSimPMTObject* PMT = new PMT8inch;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "PMT10inch"){
    WCSimPMTObject* PMT = new PMT10inch;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "PMT10inchHQE"){
    WCSimPMTObject* PMT = new PMT10inchHQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "PMT12inchHQE"){
    WCSimPMTObject* PMT = new PMT12inchHQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "HPD20inchHQE"){
    WCSimPMTObject* PMT = new HPD20inchHQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "HPD12inchHQE"){
    WCSimPMTObject* PMT = new HPD12inchHQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "BoxandLine20inchHQE"){
    WCSimPMTObject* PMT = new BoxandLine20inchHQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "BoxandLine12inchHQE"){
    WCSimPMTObject* PMT = new BoxandLine12inchHQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "FlatFacedPMT2inch"){
    WCSimPMTObject* PMT = new FlatFacedPMT2inch;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "FlatFacedPMT4inch"){
    WCSimPMTObject* PMT = new FlatFacedPMT4inch;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "PMT1cm"){
    WCSimPMTObject* PMT = new PMT1cm;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "R7081"){
    WCSimPMTObject* PMT = new PMT_R7081;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "D784KFLB"){
    WCSimPMTObject* PMT = new PMT_D784KFLB;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "R5912HQE"){
    WCSimPMTObject* PMT = new PMT_R5912HQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }
  else if (PMTType == "R7081HQE"){
    WCSimPMTObject* PMT = new PMT_R7081HQE;
    WCSimDetectorConstruction::SetPMTPointer(PMT, CollectionName);
    return PMT;
  }

  else { G4cout << PMTType << " is not a recognized PMT Type. Exiting WCSim." << G4endl; exit(1);}
}

void WCSimDetectorConstruction::UpdateCylinderGeometry()
{
  WCSimPMTObject * PMT = CreatePMTObject(cylinderTank_PMTType, WCIDCollectionName);
  WCPMTName           = PMT->GetPMTName();
  WCPMTExposeHeight   = PMT->GetExposeHeight();
  WCPMTRadius         = PMT->GetRadius();
  WCIDDiameter          = cylinderTank_Diameter;
  WCIDHeight            = cylinderTank_Height;
  WCBarrelPMTOffset     = WCPMTRadius; //offset from vertical
  WCPMTPercentCoverage  = cylinderTank_Coverage;
  WCBarrelNumPMTHorizontal = round(WCIDDiameter*sqrt(pi*WCPMTPercentCoverage)/(10.0*WCPMTRadius));
  WCBarrelNRings           = round(((WCBarrelNumPMTHorizontal*((WCIDHeight-2*WCBarrelPMTOffset)/(pi*WCIDDiameter)))
                                    /WCPMTperCellVertical));
  WCCapPMTSpacing       = (pi*WCIDDiameter/WCBarrelNumPMTHorizontal); // distance between centers of top and bottom pmts
  WCCapEdgeLimit        = WCIDDiameter/2.0 - WCPMTRadius;
  G4cout << "Cylinder height " << cylinderTank_Height << "mm, diameter " << cylinderTank_Diameter << "mm, coverage "
         << cylinderTank_Coverage << "% with " << cylinderTank_PMTType << "." << G4endl;
}

void WCSimDetectorConstruction::SaveOptionsToOutput(WCSimRootOptions * wcopt)
{
  wcopt->SetDetectorName(WCDetectorName);
  wcopt->SetSavePi0(pi0Info_isSaved);
  wcopt->SetPMTQEMethod(PMT_QE_Method);
  wcopt->SetPMTCollEff(PMT_Coll_Eff);
}
