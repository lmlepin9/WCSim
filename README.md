## Welcome to WCSim

WCSim is a very flexible GEANT4 based program for developing and
simulating large water Cherenkov detectors.

As of August 2014 WCSim has been moved to GitHub.  It can be found at:

https://github.com/WCSim

Tutorials and information about the branches and WCSim development model can be
found on the wiki:

https://github.com/WCSim/WCSim/wiki

WCSim has very few external dependencies.  It relies on only ROOT and Geant4.

There is a mailing list which will send you GitHub push/checkin
notifications here:

https://lists.phy.duke.edu/mailman/listinfo/wcsim-git

You can follow issues/requests etc by watching the GitHub respository.

## Current notes and how to build

Build Instructions:

You should have working installations of ROOT and Geant4, including the Geant4
data files such as the hadronic cross-section data.

Known working software versions:
* ROOT 6.30.06
* Geant4 10.7.4
* CMake 3.22.1
* GCC/G++ 11.4.0

To compile: 
* make clean 
* make rootcint
* make

If you want to use these libraries with an external program then also do:
* make shared      [ For root programs]
* make libWCSim.a  [ Also necessary for the event display?]

More detailed information about the simulation is available in
doc/DetectorDocumentation.pdf.

Build Instructions using CMake:

CMake is cross-platform software for managing the build process in 
a compiler-independent way (cmake.org). 
It is recommended to build ROOT and GEANT4 also through CMake. The 
latter is very CMake friendly since GEANT 4.9.6, while it started introducing
builds through CMake from 4.9.4 onwards (http://geant4.web.cern.ch/geant4/support/ReleaseNotes4.9.4.html#10.).
Using cmake, builds and source code need to well separated and make
it easier to build many versions of the same software.

A recommended way to set up the directory structure in your own
preferred WCSIM_HOME:
- ${WCSIM_HOME}/WCSim : contains the src dir, typically the cloned or 
  unzipped code from GitHub
- ${WCSIM_HOME}/WCSim_build : contains directories for each build, eg.
  for each branch you want to test or for different releases, comparing
  debugged versions, etc.
  This directory will contain the executable, the example macros and
  library for ROOT.

To compile you need to have CMakeLists.txt in the WCSim source dir.
* mkdir ${WCSIM_HOME}/WCSim_build/mydir ; cd ${WCSIM_HOME}/WCSim_build/mydir
* Set up the Geant4_Dir: export Geant4_DIR=${HOME}/Geant4/install/geant4.9.6.p04 
  (from the make install phase of Geant4)
* cmake ../../WCSim : this executes the commands in CMakeLists.txt and generates
  the Makefiles for both the ROOT library as the main executable.
* make clean : if necessary
* make : will first compile the libWCSimRoot.so which you need for using
  the ROOT Dict from WCSim and then compile WCSim.

To recompile:
* Typically just "make" will be enough and also redo the cmake phase if
  something changed.
* Sometimes you need to "make clean" first.
* When there are problems, try removing CMakeCache.txt, and redo the cmake.

Useful cmake commands:
* make edit_cache : customize the build.
* make rebuild_cache : redo the cmake phase.

Macro initialization order:
* WCSim applies `macros/preinit_geometry.mac` before `runManager->Initialize()`.
  Put detector-construction options that must affect the first geometry build
  there, for example `/WCSim/AmBe/add true`.
* The main run macro, such as `WCSim.mac`, is executed after the G4 kernel has
  already been initialized. Geometry changes made there require an explicit
  `/WCSim/Construct`, which rebuilds the geometry.

ANNIE detector component selection:
* The ANNIE tank, MRD, and FMV/FACC/veto geometry can be selected with
  `/WCSim/ANNIE/DetectorComponents`.
* Accepted values are `all`, `tank`, `mrd`, `fmv`, `facc`, `veto`, `annie`,
  and `wc`. The values `fmv`, `facc`, and `veto` select the same front veto
  geometry; `annie` and `wc` are aliases for `tank`.
* Components can be combined with commas or spaces, for example
  `/WCSim/ANNIE/DetectorComponents tank,mrd`.
* This command should be placed in `macros/preinit_geometry.mac` if it should
  affect the first geometry construction. For example:

```
/WCSim/ANNIE/DetectorComponents tank
/WCSim/AmBe/add true
/WCSim/AmBe/gdmlPath AmBeHousing.gdml
/WCSim/AmBe/center 0 0 0 cm
```

AmBe housing coordinates:
* `/WCSim/AmBe/gdmlPath` sets the GDML file used for the AmBe housing. The
  default is `AmBeHousing.gdml`, which is kept in the repository root. If WCSim
  is launched from the `build` directory, use `../AmBeHousing.gdml`.
* `/WCSim/AmBe/center` is interpreted in the local ANNIE tank water volume
  coordinate system, because the AmBe housing is placed inside `WCBarrel`.
  In this coordinate system, the tank center is `(0, 0, 0) cm`.
* The same tank center is at `(0, -14.464875, 168.1) cm` in the global WCSim
  experimental hall coordinate system. This comes from the ANNIE tank placement
  `(0, -tankyoffset, tankouterRadius + tankzoffset)`.

AmBe particle gun coordinates:
* The particle gun commands use global WCSim coordinates, not the local
  AmBe/GDML or tank coordinates used by `/WCSim/AmBe/center`.
* For ANNIEp2v7, the tank is placed at `(0, -144.64875, 1681) mm` and rotated
  by `+90 deg` about `X`. Therefore the AmBe source marker position
  `(2, 0, -168.64) mm` in tank/GDML coordinates maps to
  `(2, 23.99125, 1681) mm` for `/gun/position`.
* The BGO is downstream from the source marker in the tank/GDML `+z`
  direction. Because of the tank rotation, this corresponds to global `-y`, so
  the 4.4 MeV AmBe validation gamma should use:

```
/gun/position 2 23.99125 1681 mm
/gun/direction 0 -1 0
```

* Using the local GDML direction directly, for example `/gun/direction 0 0 1`,
  points the gamma along the wrong global axis for this placement and can miss
  the BGO response.



## Color Convention for visualization used in WCSimVismanager.cc

* gamma = green
* neutrino = yellow
* electron = blue
* positron = red
* muon = white
* muon+ = silver
* proton = magenta
* neutron = cyan

```
WCSim development is supported by the United States National Science Foundation.
```
