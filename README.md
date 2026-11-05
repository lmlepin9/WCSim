## Welcome to ANNIE WCSim

WCSim is a very flexible GEANT4 based program for developing and
simulating large water Cherenkov detectors.

## Dependencies and Environment Setup 

This package is built and run inside the ANNIE WCSim Singularity container
(`/cvmfs/singularity.opensciencegrid.org/anniesoft/wcsim:latest/`). Source
`sourceme` inside the container to set up the environment below.

| Dependency | Version in container | Install location / notes |
|---|---|---|
| OS | CentOS Linux 7.9.2009 | |
| GCC | 4.9.2 (devtoolset-3) | enabled via `/opt/rh/devtoolset-3/enable`; code is built with `-std=c++1y` |
| CMake | 2.8.12.2 | minimum required by `CMakeLists.txt`: 2.8 |
| GNU Make | 3.82 | |
| Geant4 | 10.1.3 (multithreading, GDML, OpenGL/X11; no Qt) | `/home/annie/Geant4/install` |
| Geant4 data | G4NDL4.5, G4EMLOW6.41, PhotonEvaporation3.1, RadioactiveDecay4.2, G4NEUTRONXS1.4, G4PII1.3, G4SAIDDATA1.1, G4ABLA3.0, RealSurface1.0 | `/home/annie/Geant4/install/share/data` |
| ROOT | 6.06/08 (cxx11, mathmore, xml) | `/home/annie/ROOT/install` |
| Xerces-C | 3.1.1 | system package, needed for Geant4 GDML |
| GENIE | v3 (master) | `/home/annie/Genie/GENIE-v3-master/install`; optional — `export NO_GENIE=1` before `cmake` to build without it |
| Pythia6 | 6.4.24 | `/home/annie/Pythia6Support/v6_424` (GENIE dependency) |
| LHAPDF | 5.9.1 | `/home/annie/lhapdf-5.9.1/install` (GENIE dependency) |
| log4cpp | — | `/home/annie/log4cpp/install` (GENIE dependency) |

The neutron-HP environment variables (`G4NEUTRONHP_*`) are set by
`envHadronic.sh`, which `sourceme` sources.

## Recommended build instructions 

A recommended way to set up the directory structure in your own
preferred WCSIM_HOME:

- ${WCSIM_HOME}/WCSim : contains the src dir, typically the cloned or 
  unzipped code from GitHub
- ${WCSIM_HOME}/WCSim_build : contains the executable, the example
  macros and library for ROOT.

Inside the WCSim directory run: 

* make clean 
* make rootcint
* make

If you want to use these libraries with an external program then also do:

* make shared      [ For root programs]
* make libWCSim.a  [ Also necessary for the event display?]

* cd ${WCSIM_HOME}/WCSim_build/mydir
* cmake ../../WCSim : this executes the commands in CMakeLists.txt and generates the Makefiles for both the ROOT library as the main 
executable.

* make clean : if necessary
* make : will first compile the libWCSimRoot.so which you need for using the ROOT Dict from WCSim and then compile WCSim.

To recompile:

* Typically just "make" will be enough and also redo the cmake phase if
  something changed.
* Sometimes you need to "make clean" first.
* When there are problems, try removing CMakeCache.txt, and redo the cmake.

Useful cmake commands:
* make edit_cache : customize the build.
* make rebuild_cache : redo the cmake phase.

## Color Convention for visualization used in WCSimVismanager.cc

* gamma = green
* neutrino = yellow
* electron = blue
* positron = red
* muon = white
* muon+ = silver
* proton = magenta
* neutron = cyan

## Legacy Notes 

As of August 2014 WCSim has been moved to GitHub.  It can be found at:

https://github.com/WCSim

Tutorials and information about the branches and WCSim development model can be
found on the wiki:

https://github.com/WCSim/WCSim/wiki

More detailed information about the simulation is available in
doc/DetectorDocumentation.pdf.

```
WCSim development is supported by the United States National Science Foundation.
```
