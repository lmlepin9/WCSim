#include "WCSimAmBePMTResponse.hh"

#include "TFile.h"
#include "TH1D.h"
#include "TRandom3.h"
#include "TTree.h"

#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
struct PhotonHits
{
  std::vector<double> timesNs;
  std::vector<double> energiesEv;
  std::vector<int> processes;
};

template <class T>
void WriteHistogram(const std::string& name, const std::vector<T>& values)
{
  const int numberOfBins = WCSimAmBePMTResponse::WaveformBins();
  const double binWidthNs = WCSimAmBePMTResponse::WaveformBinWidthNs();
  TH1D histogram(name.c_str(), name.c_str(), numberOfBins, 0.,
                 numberOfBins * binWidthNs);
  for(int bin = 0; bin < numberOfBins; ++bin){
    histogram.SetBinContent(bin + 1, values[bin]);
  }
  histogram.Write();
}

std::vector<int> AcceptedTimeHistogram(const std::vector<double>& timesNs)
{
  const int numberOfBins = WCSimAmBePMTResponse::WaveformBins();
  const double binWidthNs = WCSimAmBePMTResponse::WaveformBinWidthNs();
  const double shiftNs = WCSimAmBePMTResponse::TriggerAlignmentTimeNs();
  std::vector<int> histogram(numberOfBins, 0);
  for(size_t hit = 0; hit < timesNs.size(); ++hit){
    const double timeNs = timesNs[hit] + shiftNs;
    const int bin = static_cast<int>(timeNs / binWidthNs);
    if(timeNs >= 0. && bin >= 0 && bin < numberOfBins) ++histogram[bin];
  }
  return histogram;
}
}

int main(int argc, char** argv)
{
  const std::string inputPath = argc > 1 ? argv[1] : "wcsim_ambe_gamma_0.root";
  const std::string outputPath = argc > 2 ? argv[2] : "wcsim_ambe_waveforms.root";

  TFile input(inputPath.c_str(), "READ");
  if(input.IsZombie()){
    std::cerr << "Could not open input ROOT file: " << inputPath << "\n";
    return 1;
  }

  TTree* photonTree = dynamic_cast<TTree*>(input.Get("ph"));
  if(!photonTree){
    std::cerr << "Input file does not contain the AmBe PMT hit tree named 'ph'.\n";
    return 1;
  }

  int event = 0;
  int process = 0;
  double timeNs = 0.;
  double energyEv = 0.;
  photonTree->SetBranchAddress("evt", &event);
  photonTree->SetBranchAddress("t", &timeNs);
  photonTree->SetBranchAddress("e", &energyEv);
  photonTree->SetBranchAddress("process", &process);

  std::map<int, PhotonHits> photonHits;
  for(Long64_t entry = 0; entry < photonTree->GetEntries(); ++entry){
    photonTree->GetEntry(entry);
    PhotonHits& hits = photonHits[event];
    hits.timesNs.push_back(timeNs);
    hits.energiesEv.push_back(energyEv);
    hits.processes.push_back(process);
  }

  TFile output(outputPath.c_str(), "RECREATE");
  if(output.IsZombie()){
    std::cerr << "Could not create output ROOT file: " << outputPath << "\n";
    return 1;
  }

  TTree eventTree("events", "AmBe PMT response event summary");
  int eventId = 0;
  int nPhotonsEntering = 0;
  int nScintillation = 0;
  int nCherenkov = 0;
  int nOther = 0;
  int triggered = 0;
  double triggerTimeNs = -1.;
  double triggerGlobalTimeNs = -1.;
  eventTree.Branch("event", &eventId, "event/I");
  eventTree.Branch("n_photons_entering", &nPhotonsEntering,
                   "n_photons_entering/I");
  eventTree.Branch("n_scintillation", &nScintillation, "n_scintillation/I");
  eventTree.Branch("n_cherenkov", &nCherenkov, "n_cherenkov/I");
  eventTree.Branch("n_other", &nOther, "n_other/I");
  eventTree.Branch("triggered", &triggered, "triggered/I");
  eventTree.Branch("trigger_time_ns", &triggerTimeNs, "trigger_time_ns/D");
  eventTree.Branch("trigger_global_time_ns", &triggerGlobalTimeNs,
                   "trigger_global_time_ns/D");

  TRandom3 random(0);
  const long long timestamp = static_cast<long long>(std::time(0));
  for(std::map<int, PhotonHits>::const_iterator eventIt = photonHits.begin();
      eventIt != photonHits.end(); ++eventIt){
    const PhotonHits& hits = eventIt->second;
    const WCSimAmBePMTResponseResult response =
      WCSimAmBePMTResponse::Apply(hits.timesNs, hits.energiesEv,
                                  hits.processes, random);
    const std::string suffix = std::to_string(eventIt->first) + "_" +
                               std::to_string(timestamp);
    WriteHistogram("WVF_" + suffix, response.waveform);
    WriteHistogram("SCI_" + suffix,
                   AcceptedTimeHistogram(response.acceptedScintillationTimesNs));
    WriteHistogram("CHE_" + suffix,
                   AcceptedTimeHistogram(response.acceptedCherenkovTimesNs));
    if(response.triggered){
      WriteHistogram("BRF_" + suffix, response.triggerAlignedWaveform);
    }

    eventId = eventIt->first;
    nPhotonsEntering = static_cast<int>(hits.timesNs.size());
    nScintillation = response.nScintillationAccepted;
    nCherenkov = response.nCherenkovAccepted;
    nOther = response.nOtherAccepted;
    triggered = response.triggered;
    triggerTimeNs = response.triggerTimeNs;
    triggerGlobalTimeNs = response.triggerGlobalTimeNs;
    eventTree.Fill();
  }

  eventTree.Write();
  output.Close();
  input.Close();
  std::cout << "Wrote AmBe PMT response waveforms to " << outputPath << "\n";
  return 0;
}
