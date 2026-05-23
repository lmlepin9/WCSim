#include "G4ios.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "WCSimDetectorConstruction.hh"
#include "WCSimPhysicsListFactory.hh"
#include "WCSimPhysicsListFactoryMessenger.hh"
#include "WCSimTuningParameters.hh"
#include "WCSimTuningMessenger.hh"
#include "WCSimPrimaryGeneratorAction.hh"
#include "WCSimEventAction.hh"
#include "WCSimRunAction.hh"
#include "WCSimStackingAction.hh"
#include "WCSimTrackingAction.hh"
#include "WCSimSteppingAction.hh"
#include "WCSimVisManager.hh"
#if defined(G4VIS_USE) || defined(G4VIS_USE_OPENGL) || defined(G4VIS_USE_OPENGLX) || defined(G4VIS_USE_OPENGLQT)
#include "G4VisExecutive.hh"
#endif
#include "WCSimRandomParameters.hh"
#include <unistd.h>

void file_exists(const char * filename) {
  bool exists = access(filename, F_OK) != -1;
  if(!exists) {
    G4cerr << filename << " not found or inaccessible. Exiting" << G4endl;
    exit(-1);
  }
}

int main(int argc,char** argv)
{
  // Construct the default run manager
  G4RunManager* runManager = new G4RunManager;

  // get the pointer to the UI manager
  G4UImanager* UI = G4UImanager::GetUIpointer();

  // Set up the tuning parameters that need to be read before the detector
  //  construction is done
  WCSimTuningParameters* tuningpars = new WCSimTuningParameters();

  // Get the tuning parameters
  file_exists("macros/tuning_parameters.mac");
  UI->ApplyCommand("/control/execute macros/tuning_parameters.mac");

  // define random number generator parameters
  WCSimRandomParameters *randomparameters = new WCSimRandomParameters();

  // UserInitialization classes (mandatory)
  enum DetConfiguration {wfm=1,fwm=2};
  G4int WCSimConfiguration = fwm;
  G4cout<<"Making detector"<<G4endl;
  WCSimDetectorConstruction* WCSimdetector = new 
    WCSimDetectorConstruction(WCSimConfiguration,tuningpars);
  runManager->SetUserInitialization(WCSimdetector);
  G4cout<<"Making the physics"<<G4endl;
  // Added selectable physics lists 2010-07 by DMW
  // Set up the messenger hooks here, initialize the actual list after loading jobOptions.mac
  WCSimPhysicsListFactory *physFactory = new WCSimPhysicsListFactory();
  G4cout<<"Getting job options from jobOptions.mac"<<G4endl;
  // Currently, default physics list is set to FTFP_BERT
  // The custom WCSim physics list option is removed in versions later than WCSim1.6.0
  file_exists("macros/jobOptions.mac");
  UI->ApplyCommand("/control/execute macros/jobOptions.mac");
  G4cout<<"Initialising physics factories"<<G4endl;
  // Initialize the physics factory to register the selected physics.
  physFactory->InitializeList();
  runManager->SetUserInitialization(physFactory);
#if defined(G4VIS_USE) || defined(G4VIS_USE_OPENGL) || defined(G4VIS_USE_OPENGLX) || defined(G4VIS_USE_OPENGLQT)
  // Visualization
  G4cout<<"Creating visualisation manager"<<G4endl;
  //G4VisManager* visManager = new WCSimVisManager;
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();
#endif
  G4cout<<"Creating Primary Generator Action"<<G4endl;
  // Set user action classes
  WCSimPrimaryGeneratorAction* myGeneratorAction = new 
    WCSimPrimaryGeneratorAction(WCSimdetector);
  runManager->SetUserAction(myGeneratorAction);
  // read primaries from the mac file - proper way to do it
  if(access("macros/primaries_directory.mac", F_OK)!=-1){
  UI->ApplyCommand("/control/execute macros/primaries_directory.mac");
  }
  G4cout<<"Creating Run Action"<<G4endl;
  WCSimRunAction* myRunAction = new WCSimRunAction(WCSimdetector, randomparameters);

  //save all the options from WCSimTuningParameters & WCSimPhysicsListFactory
  //(set in tuning_parameters.mac & jobOptions*.mac)
  tuningpars->SaveOptionsToOutput(myRunAction->GetRootOptions());
  physFactory->SaveOptionsToOutput(myRunAction->GetRootOptions());

  runManager->SetUserAction(myRunAction);
  G4cout<<"Creating Event Action"<<G4endl;
  runManager->SetUserAction(new WCSimEventAction(myRunAction, WCSimdetector,
						 myGeneratorAction));
  G4cout<<"Creating Tracking Action"<<G4endl;
  runManager->SetUserAction(new WCSimTrackingAction);
  G4cout<<"Creating Stacking Action"<<G4endl;
  runManager->SetUserAction(new WCSimStackingAction(WCSimdetector));
  G4cout<<"Creating Stepping Action"<<G4endl;
  runManager->SetUserAction(new WCSimSteppingAction);

  if(access("macros/preinit_geometry.mac", F_OK)!=-1){
    G4cout<<"Getting pre-initialization geometry options from preinit_geometry.mac"<<G4endl;
    UI->ApplyCommand("/control/execute macros/preinit_geometry.mac");
  }

  G4cout<<"Initialising G4 kernel"<<G4endl;
  // Initialize G4 kernel
  runManager->Initialize();

  if (argc==1)   // Define UI terminal for interactive mode  
  { 

    // Start UI Session
    G4UIExecutive* session = new G4UIExecutive(argc, argv);

    G4cout<<"Executing WCSim.mac"<<G4endl;
    // Visualization Macro
    UI->ApplyCommand("/control/execute ./macros/visQtEventDisplay.mac");

    G4cout<<"Starting session"<<G4endl;
    // Start Interactive Mode
    session->SessionStart();

    delete session;
  }
  else           // Batch mode
  { 
    G4String command = "/control/execute ";
    G4String fileName = argv[1];

    UI->ApplyCommand(command+fileName);
  }

#if defined(G4VIS_USE) || defined(G4VIS_USE_OPENGL) || defined(G4VIS_USE_OPENGLX) || defined(G4VIS_USE_OPENGLQT)
  delete visManager;
#endif
  delete runManager;
  return 0;
}
