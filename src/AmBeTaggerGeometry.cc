//	-*- mode:c++; tab-width:4;	-*-
#include "AmBeTaggerGeometry.hh"
#include "WCSimDetectorConstruction.hh"

#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4Polycone.hh"
#include "G4Transform3D.hh"
#include "G4Element.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4Torus.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"
#include "G4IntersectionSolid.hh"
#include "G4Polyhedra.hh"
#include "G4LogicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4PVReplica.hh"
#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"
#include "G4AssemblyVolume.hh"
#include "G4SubtractionSolid.hh"
#include "globals.hh"
#include "G4VisAttributes.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalSurface.hh"
#include "G4UserLimits.hh"
#include "G4ReflectionFactory.hh"
#include "G4GeometryTolerance.hh"
#include "G4GeometryManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include "WCSimTuningParameters.hh" //jl145

#include <fstream>

// We develop AmBe Tagger Geometry here:
namespace
{
G4VisAttributes* MakeVisAttributes(const G4Colour& colour)
{
  auto* attributes = new G4VisAttributes(colour);
  attributes->SetVisibility(true);
  attributes->SetForceSolid(true);
  attributes->SetForceAuxEdgeVisible(true);
  return attributes;
}

// Single call site of G4MaterialPropertiesTable::AddProperty. Geant4 10.1 only
// has the array form AddProperty(key, G4double*, G4double*, G4int), so the
// g4-10.1 branch changes only this body.
void AddVectorProperty(G4MaterialPropertiesTable* mpt,
                       const char* key,
                       const std::vector<G4double>& photonEnergy,
                       const std::vector<G4double>& values)
{
  // Geant4 10.1 takes non-const arrays, so pass copies.
  std::vector<G4double> energies(photonEnergy);
  std::vector<G4double> propertyValues(values);
  mpt->AddProperty(key,
                   energies.data(),
                   propertyValues.data(),
                   static_cast<G4int>(energies.size()));
}

// Scintillation property names differ between Geant4 versions (10.1 uses
// FASTCOMPONENT/SLOWCOMPONENT, FAST/SLOWTIMECONSTANT and YIELDRATIO), so they
// are set only here. fastFraction is the fraction of the yield in component 1.
void SetScintillation(G4MaterialPropertiesTable* mpt,
                      const std::vector<G4double>& photonEnergy,
                      const std::vector<G4double>& fastSpectrum,
                      const std::vector<G4double>& slowSpectrum,
                      G4double yield,
                      G4double resolutionScale,
                      G4double fastTimeConstant,
                      G4double slowTimeConstant,
                      G4double fastFraction)
{
  // Geant4 10.1 names: component 1 is FAST, component 2 is SLOW, and
  // YIELDRATIO is the fraction of the yield in the fast component.
  AddVectorProperty(mpt, "FASTCOMPONENT", photonEnergy, fastSpectrum);
  AddVectorProperty(mpt, "SLOWCOMPONENT", photonEnergy, slowSpectrum);
  mpt->AddConstProperty("SCINTILLATIONYIELD", yield);
  mpt->AddConstProperty("RESOLUTIONSCALE", resolutionScale);
  mpt->AddConstProperty("FASTTIMECONSTANT", fastTimeConstant);
  mpt->AddConstProperty("SLOWTIMECONSTANT", slowTimeConstant);
  mpt->AddConstProperty("YIELDRATIO", fastFraction);
}

// Common photon-energy grid for the tagger optical properties.
const std::vector<G4double>& PhotonEnergy()
{
  static const std::vector<G4double> photonEnergy = {
        1.771*eV, 1.784*eV, 1.797*eV, 1.810*eV, 1.823*eV, 1.837*eV, 1.850*eV, 1.864*eV, 1.878*eV, 1.893*eV,
        1.907*eV, 1.922*eV, 1.937*eV, 1.952*eV, 1.968*eV, 1.984*eV, 2.000*eV, 2.016*eV, 2.032*eV, 2.049*eV,
        2.066*eV, 2.084*eV, 2.101*eV, 2.119*eV, 2.138*eV, 2.156*eV, 2.175*eV, 2.194*eV, 2.214*eV, 2.234*eV,
        2.254*eV, 2.275*eV, 2.296*eV, 2.317*eV, 2.339*eV, 2.362*eV, 2.384*eV, 2.407*eV, 2.431*eV, 2.455*eV,
        2.480*eV, 2.505*eV, 2.530*eV, 2.556*eV, 2.583*eV, 2.610*eV, 2.638*eV, 2.666*eV, 2.695*eV, 2.725*eV,
        2.755*eV, 2.786*eV, 2.818*eV, 2.850*eV, 2.883*eV, 2.917*eV, 2.952*eV, 2.987*eV, 3.024*eV, 3.061*eV,
        3.100*eV, 3.139*eV, 3.179*eV, 3.220*eV, 3.263*eV, 3.306*eV, 3.351*eV, 3.397*eV, 3.444*eV, 3.492*eV,
        3.542*eV, 3.594*eV, 3.646*eV, 3.701*eV, 3.757*eV, 3.815*eV, 3.874*eV, 3.936*eV, 3.999*eV, 4.065*eV,
        4.133*eV, 4.203*eV, 4.275*eV, 4.350*eV, 4.428*eV, 4.508*eV, 4.592*eV, 4.678*eV, 4.768*eV, 4.862*eV,
        4.959*eV, 5.060*eV, 5.166*eV, 5.276*eV, 5.390*eV, 5.510*eV, 5.635*eV, 5.767*eV, 5.904*eV, 6.048*eV,
        6.199*eV, 6.358*eV, 6.525*eV, 6.702*eV, 6.888*eV, 7.085*eV};
  return photonEnergy;
}

// Defines all tagger materials once. Material names carry an AmBe_ prefix so
// they cannot clash with WCSim materials (e.g. WCSim already has
// "StainlessSteel"). Materials are global and survive /WCSim/Construct, so
// Build() only calls this if they do not exist yet.
void DefineMaterials()
{
  G4NistManager* nist = G4NistManager::Instance();

  G4Material* nistAir = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* nistStainlessSteel =
      nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  G4Material* nistPolyvinylChloride =
      nist->FindOrBuildMaterial("G4_POLYVINYL_CHLORIDE");
  G4Element* hydrogen = nist->FindOrBuildElement("H");
  G4Element* carbon = nist->FindOrBuildElement("C");
  G4Element* nitrogen = nist->FindOrBuildElement("N");
  G4Element* bismuth = nist->FindOrBuildElement("Bi");
  G4Element* germanium = nist->FindOrBuildElement("Ge");
  G4Element* oxygen = nist->FindOrBuildElement("O");

  const std::vector<G4double>& photonEnergy = PhotonEnergy();

  const G4double bgoDensity = 7.13 * g / cm3; 

  G4Material* bgo = new G4Material(
    "AmBe_BGO",
    bgoDensity,
    3);

  bgo->AddElement(bismuth,4);
  bgo->AddElement(germanium,3);
  bgo->AddElement(oxygen,12);

  const G4double bgoBirksConstant = 0.008415 * mm / MeV;
  bgo->GetIonisation()->SetBirksConstant(bgoBirksConstant);



  const std::vector<G4double> bgoRefractiveIndex = {
        2.0843, 2.0849, 2.0858, 2.0871, 2.0877, 2.0885, 2.0900, 2.0906, 2.0919, 2.0928,
        2.0938, 2.0949, 2.0956, 2.0969, 2.0984, 2.0990, 2.1003, 2.1018, 2.1032, 2.1045,
        2.1060, 2.1072, 2.1089, 2.1102, 2.1117, 2.1130, 2.1149, 2.1166, 2.1183, 2.1201,
        2.1218, 2.1235, 2.1257, 2.1276, 2.1296, 2.1320, 2.1339, 2.1362, 2.1385, 2.1410,
        2.1435, 2.1460, 2.1488, 2.1518, 2.1543, 2.1576, 2.1608, 2.1640, 2.1672, 2.1709,
        2.1744, 2.1783, 2.1823, 2.1865, 2.1912, 2.1957, 2.2001, 2.2051, 2.2103, 2.2156,
        2.2213, 2.2272, 2.2335, 2.2407, 2.2480, 2.2550, 2.2626, 2.2708, 2.2795, 2.2889,
        2.2984, 2.3089, 2.3196, 2.3311, 2.3437, 2.3570, 2.3715, 2.3867, 2.4032, 2.4159,
        2.4303, 2.4447, 2.4590, 2.4734, 2.4878, 2.5022, 2.5166, 2.5310, 2.5454, 2.5598,
        2.5742, 2.5885, 2.6029, 2.6173, 2.6317, 2.6461, 2.6605, 2.6749, 2.6893, 2.7037,
        2.7180, 2.7324, 2.7468, 2.7612, 2.7756, 2.7900};

  const std::vector<G4double> bgoScintillationFast =  {
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0826, 0.0943, 0.1107, 0.1316, 0.1504, 0.1712, 0.1901, 0.2109, 0.2307,
        0.2510, 0.2785, 0.3060, 0.3399, 0.3800, 0.4184, 0.4487, 0.4763, 0.5089, 0.5449,
        0.5810, 0.6244, 0.6682, 0.7102, 0.7504, 0.7905, 0.8301, 0.8709, 0.8979, 0.9094,
        0.9220, 0.9492, 0.9769, 0.9913, 0.9913, 0.9911, 0.9762, 0.9602, 0.9362, 0.9036,
        0.8704, 0.8142, 0.7525, 0.6948, 0.6422, 0.5885, 0.5065, 0.4223, 0.3542, 0.3019,
        0.2496, 0.1899, 0.1304, 0.0796, 0.0400, 0.0037, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};

  const std::vector<G4double> bgoScintillationSlow = {
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0826, 0.0943, 0.1107, 0.1316, 0.1504, 0.1712, 0.1901, 0.2109, 0.2307,
        0.2510, 0.2785, 0.3060, 0.3399, 0.3800, 0.4184, 0.4487, 0.4763, 0.5089, 0.5449,
        0.5810, 0.6244, 0.6682, 0.7102, 0.7504, 0.7905, 0.8301, 0.8709, 0.8979, 0.9094,
        0.9220, 0.9492, 0.9769, 0.9913, 0.9913, 0.9911, 0.9762, 0.9602, 0.9362, 0.9036,
        0.8704, 0.8142, 0.7525, 0.6948, 0.6422, 0.5885, 0.5065, 0.4223, 0.3542, 0.3019,
        0.2496, 0.1899, 0.1304, 0.0796, 0.0400, 0.0037, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};


  const std::vector<G4double> bgoAbsorptionLength = {
        10.05*cm, 10.03*cm, 10.01*cm, 9.99*cm, 10.04*cm, 10.04*cm, 9.93*cm, 9.93*cm, 9.92*cm, 9.85*cm,
         9.95*cm,  9.92*cm,  9.79*cm, 9.85*cm,  9.87*cm,  9.86*cm, 9.78*cm, 9.83*cm, 9.80*cm, 9.75*cm,
         9.70*cm,  9.72*cm,  9.65*cm, 9.76*cm,  9.73*cm,  9.70*cm, 9.68*cm, 9.65*cm, 9.70*cm, 9.70*cm,
         9.65*cm,  9.58*cm,  9.65*cm, 9.64*cm,  9.60*cm,  9.51*cm, 9.47*cm, 9.47*cm, 9.42*cm, 9.43*cm,
         9.40*cm,  9.39*cm,  9.33*cm, 9.39*cm,  9.33*cm,  9.26*cm, 9.25*cm, 9.17*cm, 9.13*cm, 9.08*cm,
         9.08*cm,  9.06*cm,  8.95*cm, 8.89*cm,  8.86*cm,  8.90*cm, 8.82*cm, 8.71*cm, 8.72*cm, 8.60*cm,
         8.53*cm,  8.40*cm,  8.45*cm, 8.25*cm,  8.22*cm,  7.97*cm, 7.70*cm, 7.76*cm, 7.54*cm, 7.30*cm,
         7.06*cm,  6.71*cm,  6.27*cm, 5.98*cm,  5.52*cm,  4.94*cm, 4.15*cm, 2.60*cm, 1.06*cm, 0.54*cm,
         1e-6*cm,  1e-6*cm,  1e-6*cm, 1e-6*cm,  1e-6*cm,  1e-6*cm, 1e-6*cm, 1e-6*cm, 1e-6*cm, 1e-6*cm,
         1e-6*cm,  1e-6*cm,  1e-6*cm, 1e-6*cm,  1e-6*cm,  1e-6*cm, 1e-6*cm, 1e-6*cm, 1e-6*cm, 1e-6*cm,
         1e-6*cm,  1e-6*cm,  1e-6*cm, 1e-6*cm,  1e-6*cm, 1e-6*cm};

  const G4double bgoLightYield = 10000. / MeV;
  constexpr G4double bgoResolutionScale = 2.0;
  const G4double bgoFastTimeConstant = 1.0 * ns;
  const G4double bgoSlowTimeConstant = 300.0 * ns;
  constexpr G4double bgoFastFraction = 0.0;

  G4MaterialPropertiesTable* bgoMpt = new G4MaterialPropertiesTable;


  AddVectorProperty(bgoMpt, "ABSLENGTH", photonEnergy, bgoAbsorptionLength);
  AddVectorProperty(bgoMpt, "RINDEX", photonEnergy, bgoRefractiveIndex);
  SetScintillation(
      bgoMpt,
      photonEnergy,
      bgoScintillationFast,
      bgoScintillationSlow,
      bgoLightYield,
      bgoResolutionScale,
      bgoFastTimeConstant,
      bgoSlowTimeConstant,
      bgoFastFraction);

  bgo->SetMaterialPropertiesTable(bgoMpt);

  const auto n_entries = photonEnergy.size();
  const std::vector<G4double> airRefractiveIndex(n_entries,1.0);
  const std::vector<G4double> airAbsorptionLength(n_entries,100*m);


  // Own copy of G4_AIR so the optical properties do not leak into other
  // WCSim volumes made of G4_AIR.
  G4Material* air = new G4Material("AmBe_Air", nistAir->GetDensity(), 1);
  air->AddMaterial(nistAir, 1.0);

  G4MaterialPropertiesTable* airMpt = new G4MaterialPropertiesTable;
  AddVectorProperty(airMpt, "RINDEX", photonEnergy, airRefractiveIndex);
  AddVectorProperty(airMpt, "ABSLENGTH", photonEnergy, airAbsorptionLength);
  air->SetMaterialPropertiesTable(airMpt);

  const G4double polyurethaneFoamDensity = 0.032 * g / cm3;

  G4Material* polyurethaneFoam = new G4Material(
      "AmBe_PolyurethaneFoam",
      polyurethaneFoamDensity,
      4);
  polyurethaneFoam->AddElement(hydrogen, 0.060);
  polyurethaneFoam->AddElement(carbon, 0.630);
  polyurethaneFoam->AddElement(nitrogen, 0.060);
  polyurethaneFoam->AddElement(oxygen, 0.250);

  const G4double stainlessSteelDensity = 8.0 * g / cm3;

  G4Material* stainlessSteel = new G4Material(
      "AmBe_StainlessSteel",
      stainlessSteelDensity,
      1);
  stainlessSteel->AddMaterial(nistStainlessSteel, 1.0);

  const G4double whitePvcDensity = 1.38 * g / cm3;

  G4Material* whitePvc = new G4Material(
      "AmBe_WhitePVC",
      whitePvcDensity,
      1);
  whitePvc->AddMaterial(nistPolyvinylChloride, 1.0);

  // Bulk optical properties let photons enter PVC without NoRINDEX; the short
  // absorption length is provisional and makes the material effectively opaque.
  constexpr G4double whitePvcRefractiveIndex = 1.54;
  const G4double whitePvcBulkAbsorptionLength = 0.1 * mm;
  const std::vector<G4double> whitePvcRefractiveIndexValues(
      n_entries,
      whitePvcRefractiveIndex);
  const std::vector<G4double> whitePvcAbsorptionLengthValues(
      n_entries,
      whitePvcBulkAbsorptionLength);

  G4MaterialPropertiesTable* whitePvcMaterialMpt =
      new G4MaterialPropertiesTable;
  AddVectorProperty(
      whitePvcMaterialMpt,
      "RINDEX",
      photonEnergy,
      whitePvcRefractiveIndexValues);
  AddVectorProperty(
      whitePvcMaterialMpt,
      "ABSLENGTH",
      photonEnergy,
      whitePvcAbsorptionLengthValues);
  whitePvc->SetMaterialPropertiesTable(whitePvcMaterialMpt);

  // PMT body of the BGOg4Sim reference model, PTFE with RINDEX 1.35 and a
  // 1e-6 m absorption length, so photons refract in and are counted on entry.
  // Own copy of G4_TEFLON so these properties do not reach the BGO wrap.
  G4Material* nistTeflon = nist->FindOrBuildMaterial("G4_TEFLON");
  G4Material* referencePmtPtfe = new G4Material(
      "AmBe_ReferencePmtPTFE",
      nistTeflon->GetDensity(),
      1);
  referencePmtPtfe->AddMaterial(nistTeflon, 1.0);

  const std::vector<G4double> referencePmtRefractiveIndex(n_entries, 1.35);
  const std::vector<G4double> referencePmtAbsorptionLength(n_entries, 1e-6 * m);
  G4MaterialPropertiesTable* referencePmtMpt = new G4MaterialPropertiesTable;
  AddVectorProperty(
      referencePmtMpt,
      "RINDEX",
      photonEnergy,
      referencePmtRefractiveIndex);
  AddVectorProperty(
      referencePmtMpt,
      "ABSLENGTH",
      photonEnergy,
      referencePmtAbsorptionLength);
  referencePmtPtfe->SetMaterialPropertiesTable(referencePmtMpt);
}
}

G4LogicalVolume* AmBeTaggerGeometry::Build(
    G4LogicalVolume* mother,
    const G4ThreeVector& pos,
    const G4RotationMatrix& rot)
{
  if (G4Material::GetMaterial("AmBe_BGO", false) == nullptr) {
    DefineMaterials();
  }

  G4Material* air = G4Material::GetMaterial("AmBe_Air");
  G4Material* bgo = G4Material::GetMaterial("AmBe_BGO");
  G4Material* polyurethaneFoam =
      G4Material::GetMaterial("AmBe_PolyurethaneFoam");
  G4Material* stainlessSteel = G4Material::GetMaterial("AmBe_StainlessSteel");
  G4Material* whitePvc = G4Material::GetMaterial("AmBe_WhitePVC");
  G4Material* teflon =
      G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON");
  G4Material* referencePmtPtfe =
      G4Material::GetMaterial("AmBe_ReferencePmtPTFE");

  const std::vector<G4double>& photonEnergy = PhotonEnergy();
  const auto n_entries = photonEnergy.size();

  // Optical surfaces are rebuilt with the geometry (WCSim cleans the surface
  // tables in UpdateGeometry).
  constexpr G4double whitePvcReflectivity = 0.80;
  const std::vector<G4double> whitePvcReflectivityValues(
      n_entries,
      whitePvcReflectivity);
  const std::vector<G4double> whitePvcTransmittanceValues(n_entries, 0.0);
  const std::vector<G4double> whitePvcEfficiencyValues(n_entries, 0.0);

  // Surface reflectivity is an effective painted-PVC model. The 0.80 value is
  // a provisional calibration parameter, not a measured property of this PVC.
  G4MaterialPropertiesTable* whitePvcSurfaceMpt =
      new G4MaterialPropertiesTable;
  AddVectorProperty(
      whitePvcSurfaceMpt,
      "REFLECTIVITY",
      photonEnergy,
      whitePvcReflectivityValues);
  AddVectorProperty(
      whitePvcSurfaceMpt,
      "TRANSMITTANCE",
      photonEnergy,
      whitePvcTransmittanceValues);
  AddVectorProperty(
      whitePvcSurfaceMpt,
      "EFFICIENCY",
      photonEnergy,
      whitePvcEfficiencyValues);

  G4OpticalSurface* whitePvcSurface =
      new G4OpticalSurface("WhitePvcReflectiveSurface");
  whitePvcSurface->SetType(dielectric_dielectric);
  whitePvcSurface->SetModel(unified);
  whitePvcSurface->SetFinish(groundfrontpainted);
  whitePvcSurface->SetMaterialPropertiesTable(whitePvcSurfaceMpt);


  // Housing dimensions, in the tagger frame (BGO centre at z = 0, PMT at +z).
  // Defined first because the housing and cavity must exist before any
  // daughter is placed.
  const G4double bgoHalfLengthZ = 2.5 * cm;

  // PTFE wrap around the BGO (barrel and both caps). The reflection is set by
  // the wrapSurface REFLECTIVITY, not by this thickness; it is kept thin
  // (the front cap is a ring around the PMT front face).
  const G4double wrapThickness = 0.01 * mm;

  const G4double sourceShieldRadius = 26.025 * mm;
  const G4double sourceShieldHalfThickness = 0.5 * 11.63 * mm;
  const G4double sourceCapsuleHalfLength = 5.0 * mm;
  // Steel disk flush against the PTFE back cap.
  const G4double sourceShieldFrontZ =
      -bgoHalfLengthZ - wrapThickness;
  const G4double sourceShieldZ =
      sourceShieldFrontZ - sourceShieldHalfThickness;
  const G4double sourceShieldBackZ =
      sourceShieldZ - sourceShieldHalfThickness;
  const G4double sourceCapsuleZ =
      sourceShieldBackZ - sourceCapsuleHalfLength;

  const G4double outerPvcInnerRadius = sourceShieldRadius;
  const G4double outerPvcOuterRadius = 35.235 * mm;
  const G4double outerPvcBarrelHalfLength = 190.0 * mm;
  const G4double rearEndCapHalfLength = 17.365 * mm;
  const G4double rearEndCapSourceTubeLength = 11.63 * mm;
  const G4double rearEndCapInsertionStartZ = 2.636 * mm;
  const G4double rearEndCapFrontZ =
      sourceCapsuleZ - sourceCapsuleHalfLength;
  const G4double rearEndCapZ =
      rearEndCapFrontZ - rearEndCapHalfLength - rearEndCapSourceTubeLength;
  const G4double outerPvcBarrelBackZ =
      rearEndCapZ + rearEndCapInsertionStartZ;
  const G4double outerPvcBarrelZ =
      outerPvcBarrelBackZ + outerPvcBarrelHalfLength;
  const G4double outerPvcBarrelFrontZ =
      outerPvcBarrelZ + outerPvcBarrelHalfLength;

  const G4double frontEndCapHalfLength = 15.725 * mm;
  const G4double frontEndCapConeStartLocalZ = -9.075 * mm;
  const G4double frontEndCapTipRadius = 19.575 * mm;
  const G4double frontEndCapZ =
      outerPvcBarrelFrontZ + frontEndCapHalfLength;

  // Housing: one PVC solid with the outer profile (rear cap -> barrel -> cone).
  // It replaces the separate barrel / rear-cap / front-cap volumes and is the
  // only volume placed in the mother.
  const G4double housingBackZ = rearEndCapZ - rearEndCapHalfLength;
  const G4double housingConeStartZ = frontEndCapZ + frontEndCapConeStartLocalZ;
  const G4double housingFrontZ = frontEndCapZ + frontEndCapHalfLength;

  const std::vector<G4double> housingZPlanes = {
      housingBackZ,
      housingConeStartZ,
      housingFrontZ};
  const std::vector<G4double> housingInnerRadii(housingZPlanes.size(), 0.0 * mm);
  const std::vector<G4double> housingOuterRadii = {
      outerPvcOuterRadius,
      outerPvcOuterRadius,
      frontEndCapTipRadius};

  G4Polycone* housingSolid = new G4Polycone(
      "AmBe_HousingSolid",
      0.0 * deg,
      360.0 * deg,
      static_cast<G4int>(housingZPlanes.size()),
      housingZPlanes.data(),
      housingInnerRadii.data(),
      housingOuterRadii.data());

  const G4Colour whitePvcColour(0.96, 0.96, 0.90, 0.30);

  G4LogicalVolume* housingLogical = new G4LogicalVolume(
      housingSolid,
      whitePvc,
      "AmBe_HousingLogical");
  housingLogical->SetVisAttributes(MakeVisAttributes(whitePvcColour));
  new G4LogicalSkinSurface(
      "AmBe_HousingSurface",
      housingLogical,
      whitePvcSurface);

  // The Transform3D constructor copies the rotation, so the caller's matrix
  // does not need to outlive this call. rot is the active rotation of the
  // tagger in the mother frame.
  new G4PVPlacement(
      G4Transform3D(rot, pos),
      housingLogical,
      "AmBe_HousingPhysical",
      mother,
      false,
      0,
      true);

  // Cavity: air inside the barrel, from the rear-cap front face to the
  // front-cap back face. All internal parts are placed here.
  const G4double cavityBackZ = rearEndCapFrontZ;
  const G4double cavityFrontZ = outerPvcBarrelFrontZ;
  const G4double cavityHalfLength = 0.5 * (cavityFrontZ - cavityBackZ);
  const G4double cavityCentreZ = 0.5 * (cavityFrontZ + cavityBackZ);

  G4Tubs* cavitySolid = new G4Tubs(
      "AmBe_CavitySolid",
      0.0 * mm,
      outerPvcInnerRadius,
      cavityHalfLength,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* cavityLogical = new G4LogicalVolume(
      cavitySolid,
      air,
      "AmBe_CavityLogical");
  cavityLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

  G4VPhysicalVolume* cavityPhysical = new G4PVPlacement(
      nullptr,
      G4ThreeVector(0.0, 0.0, cavityCentreZ),
      cavityLogical,
      "AmBe_CavityPhysical",
      housingLogical,
      false,
      0,
      true);

  // Daughters keep their tagger-frame z (BGO centre at 0); this converts it
  // to the cavity frame.
  const auto inCavity = [cavityCentreZ](G4double taggerZ) {
    return G4ThreeVector(0.0, 0.0, taggerZ - cavityCentreZ);
  };

   const G4double bgoMaxRadius = 2.5 * cm;
   const G4double bgoMinRadius = 1.0 * mm;


   G4Tubs* bgoSolid = new G4Tubs(
    "bgoSolid",
    bgoMinRadius,
    bgoMaxRadius,
    bgoHalfLengthZ,
    0.0 * deg,
    360.0 * deg);

   scoringVolume_ = new G4LogicalVolume(
        bgoSolid,
        bgo,
        "bgoLogical");
   scoringVolume_->SetVisAttributes(MakeVisAttributes(G4Colour(0.05, 0.25, 1.0, 0.38)));

  G4VPhysicalVolume* bgoPhysical = new G4PVPlacement(
    nullptr,
    inCavity(0.0),
    scoringVolume_,
    "bgoPhysical",
    cavityLogical,
    false,
    0,
    true);

  const G4Colour stainlessSteelColour(0.58, 0.60, 0.63, 0.76);
  const G4double sourceCapsuleOuterRadius = 7.8 * mm;
  const G4double sourceCapsuleInnerRadius = 6.8 * mm;
  const G4double sourceSurroundingShieldInnerRadius = 8.0 * mm;

  G4Tubs* sourceShieldSolid = new G4Tubs(
      "SourceDownstreamShieldSolid",
      0.0 * cm,
      sourceShieldRadius,
      sourceShieldHalfThickness,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* sourceShieldLogical = new G4LogicalVolume(
      sourceShieldSolid,
      stainlessSteel,
      "SourceDownstreamShieldLogical");
  sourceShieldLogical->SetVisAttributes(
      MakeVisAttributes(stainlessSteelColour));

  new G4PVPlacement(
      nullptr,
      inCavity(sourceShieldZ),
      sourceShieldLogical,
      "SourceDownstreamShieldPhysical",
      cavityLogical,
      false,
      0,
      true);

  const std::vector<G4double> capsuleZPlanes = {
      -5.0 * mm,
      -4.0 * mm,
      -3.9999 * mm,
      3.9999 * mm,
      4.0 * mm,
      5.0 * mm};
  const std::vector<G4double> capsuleInnerRadii = {
      0.0 * mm,
      0.0 * mm,
      sourceCapsuleInnerRadius,
      sourceCapsuleInnerRadius,
      0.0 * mm,
      0.0 * mm};
  const std::vector<G4double> capsuleOuterRadii = {
      sourceCapsuleOuterRadius,
      sourceCapsuleOuterRadius,
      sourceCapsuleOuterRadius,
      sourceCapsuleOuterRadius,
      sourceCapsuleOuterRadius,
      sourceCapsuleOuterRadius};

  G4Polycone* sourceCapsuleSolid = new G4Polycone(
      "SourceCapsuleSolid",
      0.0 * deg,
      360.0 * deg,
      static_cast<G4int>(capsuleZPlanes.size()),
      capsuleZPlanes.data(),
      capsuleInnerRadii.data(),
      capsuleOuterRadii.data());

  G4LogicalVolume* sourceCapsuleLogical = new G4LogicalVolume(
      sourceCapsuleSolid,
      stainlessSteel,
      "SourceCapsuleLogical");
  sourceCapsuleLogical->SetVisAttributes(
      MakeVisAttributes(stainlessSteelColour));

  new G4PVPlacement(
      nullptr,
      inCavity(sourceCapsuleZ),
      sourceCapsuleLogical,
      "SourceCapsulePhysical",
      cavityLogical,
      false,
      0,
      true);

  G4Tubs* sourceSurroundingShieldSolid = new G4Tubs(
      "SourceSurroundingShieldSolid",
      sourceSurroundingShieldInnerRadius,
      sourceShieldRadius,
      sourceCapsuleHalfLength,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* sourceSurroundingShieldLogical = new G4LogicalVolume(
      sourceSurroundingShieldSolid,
      stainlessSteel,
      "SourceSurroundingShieldLogical");
  sourceSurroundingShieldLogical->SetVisAttributes(
      MakeVisAttributes(stainlessSteelColour));

  new G4PVPlacement(
      nullptr,
      inCavity(sourceCapsuleZ),
      sourceSurroundingShieldLogical,
      "SourceSurroundingShieldPhysical",
      cavityLogical,
      false,
      0,
      true);


  
  //const std::vector<G4double> tefRefractiveIndex(n_entries,1.35);
  //const std::vector<G4double> tefAbsorptionLength(n_entries,1e-6*m);
  const std::vector<G4double> tefReflectance(n_entries,0.9);
  const std::vector<G4double> tefEfficiency(n_entries,0.0);

  G4MaterialPropertiesTable* wrapMpt = new G4MaterialPropertiesTable;
  AddVectorProperty(wrapMpt, "REFLECTIVITY", photonEnergy, tefReflectance);
  //AddVectorProperty(wrapMpt, "ABSLENGTH", photonEnergy, tefAbsorptionLength);
  //AddVectorProperty(wrapMpt, "RINDEX",photonEnergy, tefRefractiveIndex);
  AddVectorProperty(wrapMpt, "EFFICIENCY", photonEnergy, tefEfficiency);

  G4OpticalSurface* wrapSurface = new G4OpticalSurface("BgoWrapSurface");
  wrapSurface->SetType(dielectric_metal);
  wrapSurface->SetFinish(ground);
  wrapSurface->SetModel(glisur);
  wrapSurface->SetMaterialPropertiesTable(wrapMpt);

  const G4double wrapOuterRadius = bgoMaxRadius + wrapThickness;
  const G4double wrapHalfThickness = 0.5 * wrapThickness;
  // BGOg4Sim PMT radius; the front cap opens only over the PMT, as the
  // reference tape top disk does.
  const G4double referencePmtRadius = 12.25 * mm;
  const G4double frontCapInnerRadius = referencePmtRadius;

  G4Tubs* wrapBarrelSolid = new G4Tubs(
      "BgoTeflonBarrelWrapSolid",
      bgoMaxRadius,
      wrapOuterRadius,
      bgoHalfLengthZ,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* wrapBarrelLogical = new G4LogicalVolume(
      wrapBarrelSolid,
      teflon,
      "BgoTeflonBarrelWrapLogical");
  wrapBarrelLogical->SetVisAttributes(
      MakeVisAttributes(G4Colour(0.92, 0.92, 0.86, 0.72)));

  G4VPhysicalVolume* wrapBarrelPhysical = new G4PVPlacement(
      nullptr,
      inCavity(0.0),
      wrapBarrelLogical,
      "BgoTeflonBarrelWrapPhysical",
      cavityLogical,
      false,
      0,
      true);

  G4Tubs* wrapBackCapSolid = new G4Tubs(
      "BgoTeflonBackCapSolid",
      0.0 * cm,
      wrapOuterRadius,
      wrapHalfThickness,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* wrapBackCapLogical = new G4LogicalVolume(
      wrapBackCapSolid,
      teflon,
      "BgoTeflonBackCapLogical");
  wrapBackCapLogical->SetVisAttributes(
      MakeVisAttributes(G4Colour(0.92, 0.92, 0.86, 0.72)));

  G4VPhysicalVolume* wrapBackCapPhysical = new G4PVPlacement(
      nullptr,
      inCavity(-bgoHalfLengthZ - wrapHalfThickness),
      wrapBackCapLogical,
      "BgoTeflonBackCapPhysical",
      cavityLogical,
      false,
      0,
      true);

  G4Tubs* wrapFrontCapSolid = new G4Tubs(
      "BgoTeflonFrontCapSolid",
      frontCapInnerRadius,
      wrapOuterRadius,
      wrapHalfThickness,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* wrapFrontCapLogical = new G4LogicalVolume(
      wrapFrontCapSolid,
      teflon,
      "BgoTeflonFrontCapLogical");
  wrapFrontCapLogical->SetVisAttributes(
      MakeVisAttributes(G4Colour(0.92, 0.92, 0.86, 0.72)));

  G4VPhysicalVolume* wrapFrontCapPhysical = new G4PVPlacement(
      nullptr,
      inCavity(bgoHalfLengthZ + wrapHalfThickness),
      wrapFrontCapLogical,
      "BgoTeflonFrontCapPhysical",
      cavityLogical,
      false,
      0,
      true);

  new G4LogicalBorderSurface(
      "BgoToTeflonBarrelWrapSurface",
      bgoPhysical,
      wrapBarrelPhysical,
      wrapSurface);

  new G4LogicalBorderSurface(
      "BgoToTeflonBackCapSurface",
      bgoPhysical,
      wrapBackCapPhysical,
      wrapSurface);

  new G4LogicalBorderSurface(
      "BgoToTeflonFrontCapSurface",
      bgoPhysical,
      wrapFrontCapPhysical,
      wrapSurface);


  // PMT: BGOg4Sim reference model, a PTFE cylinder (RINDEX 1.35, opaque) of
  // radius 12.25 mm and length 100 mm whose front face sits referencePmtGap
  // above the BGO front face (cavity air in between). It is the readout
  // volume: TaggerPmtReadout counts photons refracted into it and applies the
  // QE. No coupling, window, envelope or vacuum is modelled.
  // Gap: BGOg4Sim takes it from the command line (default 1 mm; its
  // published runs, exec.sh, use 0.1 mm). Here 0 mm: the PMT touches the
  // BGO. Note BGOg4Sim at 0 mm also counts photons reflected at the PMT
  // face (post-step volume check), inflating its light ~3x; we count
  // refracted photons only.
  const G4double bgoFrontZ = bgoHalfLengthZ;
  const G4double referencePmtGap = 0.0 * mm;
  const G4double referencePmtHalfLength = 50.0 * mm;
  const G4double referencePmtZ =
      bgoFrontZ + referencePmtGap + referencePmtHalfLength;

  G4Tubs* referencePmtSolid = new G4Tubs(
      "ReferencePmtSolid",
      0.0 * cm,
      referencePmtRadius,
      referencePmtHalfLength,
      0.0 * deg,
      360.0 * deg);

  pmtPlaneVolume_ = new G4LogicalVolume(
      referencePmtSolid,
      referencePmtPtfe,
      "ReferencePmtLogical");
  pmtPlaneVolume_->SetVisAttributes(
      MakeVisAttributes(G4Colour(1.0, 0.5, 0.5, 0.5)));

  new G4PVPlacement(
      nullptr,
      inCavity(referencePmtZ),
      pmtPlaneVolume_,
      "ReferencePmtPhysical",
      cavityLogical,
      false,
      0,
      true);

  // The readout PVC holder keeps the placement it had with the former
  // R1398-04 model: its front face is 101.1 mm above the BGO front face.
  const G4double readoutPvcHolderFrontZ = bgoFrontZ + 101.1 * mm;

  // Black foam support around the PMT: a sleeve in contact with the PMT
  // barrel, from the top of the PTFE front cap up to the readout holder, and
  // out to the cavity wall. The foam has no RINDEX, so optical photons that
  // reach it are absorbed.
  const G4double pmtFoamInnerRadius = referencePmtRadius;
  const G4double pmtFoamOuterRadius = outerPvcInnerRadius;
  const G4double pmtFoamBackZ = bgoFrontZ + wrapThickness;
  const G4double pmtFoamHalfLength =
      0.5 * (readoutPvcHolderFrontZ - pmtFoamBackZ);
  const G4double pmtFoamZ = pmtFoamBackZ + pmtFoamHalfLength;

  G4Tubs* pmtFoamSolid = new G4Tubs(
      "PmtFoamSupportSolid",
      pmtFoamInnerRadius,
      pmtFoamOuterRadius,
      pmtFoamHalfLength,
      0.0 * deg,
      360.0 * deg);

  G4LogicalVolume* pmtFoamLogical = new G4LogicalVolume(
      pmtFoamSolid,
      polyurethaneFoam,
      "PmtFoamSupportLogical");
  pmtFoamLogical->SetVisAttributes(
      MakeVisAttributes(G4Colour(0.04, 0.045, 0.055, 0.28)));

  new G4PVPlacement(
      nullptr,
      inCavity(pmtFoamZ),
      pmtFoamLogical,
      "PmtFoamSupportPhysical",
      cavityLogical,
      false,
      0,
      true);

  const G4double readoutPvcHolderHalfLength = 60.0 * mm;
  const G4double readoutPvcHolderZ =
      readoutPvcHolderFrontZ + readoutPvcHolderHalfLength;
  // Repeated z planes give flat annular steps (a 1 mm lip of inner radius
  // 12.79 mm between local z = -40 and -39 mm). Near-zero-length conical
  // sections here made photons reflected near the inner corners cross a few
  // um of PVC, which the boundary check in WCSimSteppingAction reported.
  const std::vector<G4double> readoutPvcHolderZPlanes = {
      -60.0 * mm,
      -40.0 * mm,
      -40.0 * mm,
      -39.0 * mm,
      -39.0 * mm,
      60.0 * mm};
  const std::vector<G4double> readoutPvcHolderInnerRadii = {
      25.0 * mm,
      25.0 * mm,
      12.79 * mm,
      12.79 * mm,
      25.0 * mm,
      25.0 * mm};
  const std::vector<G4double> readoutPvcHolderOuterRadii = {
      sourceShieldRadius,
      sourceShieldRadius,
      sourceShieldRadius,
      sourceShieldRadius,
      sourceShieldRadius,
      sourceShieldRadius};

  G4Polycone* readoutPvcHolderSolid = new G4Polycone(
      "ReadoutPvcHolderSolid",
      0.0 * deg,
      360.0 * deg,
      static_cast<G4int>(readoutPvcHolderZPlanes.size()),
      readoutPvcHolderZPlanes.data(),
      readoutPvcHolderInnerRadii.data(),
      readoutPvcHolderOuterRadii.data());

  G4LogicalVolume* readoutPvcHolderLogical = new G4LogicalVolume(
      readoutPvcHolderSolid,
      whitePvc,
      "ReadoutPvcHolderLogical");
  readoutPvcHolderLogical->SetVisAttributes(
      MakeVisAttributes(whitePvcColour));
  new G4LogicalSkinSurface(
      "ReadoutPvcHolderSurface",
      readoutPvcHolderLogical,
      whitePvcSurface);

  new G4PVPlacement(
      nullptr,
      inCavity(readoutPvcHolderZ),
      readoutPvcHolderLogical,
      "ReadoutPvcHolderPhysical",
      cavityLogical,
      false,
      0,
      true);

  return housingLogical;
}
