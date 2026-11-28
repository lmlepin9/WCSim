// WCSim counterpart of BGOg4Sim's scripts/read_waveforms.C for the AmBe tagger
// waveforms (tagger_wf). It makes the same three canvases:
//   <outprefix>_single.png   one random triggered waveform
//   <outprefix>_few.png      3x3 random triggered waveforms with maximum >= 30 mV
//   <outprefix>_fprompt.png  F_prompt vs total charge (200 x 120 bins,
//                            charge -1..8, F_prompt 0..1.2, stats box)
// and writes the 2D histogram to <outprefix>.root.
//
// Steps, as in BGOg4Sim (photon_histogram_builder.py + read_waveforms.C):
//   trigger   an event is kept only if its waveform crosses triggerThreshold
//             (10 mV, the trigger_threshold of photon_histogram_builder.py);
//             BGOg4Sim writes BRF_* waveforms only for these events.
//   alignment with alignTrigger, the waveform is shifted so that the first
//             sample above threshold sits at 700 ns, as for the BRF_* waveforms
//             (samples shifted in are set to the baseline). Without it the
//             stored WCSim time axis is used (global time 0 = 700 ns).
//   baseline  SubtractBaseline(waveform, 3): iterative +-1 sigma clipping over
//             the whole (shifted) waveform, mean of what is left.
//   integrals TH1::Integral(FindBin(t1), FindBin(t2)) of the baseline-subtracted
//             waveform: prompt 690-850 ns, total 690-900 ns (plain bin sums,
//             both ends inclusive, no dt factor); F_prompt = prompt / total,
//             events with total <= 0 skipped.
//
// Units. WCSim stores mV; BGOg4Sim waveforms are in V. ampScale = 0.001 (the
// default) gives V, like the BGOg4Sim histograms. The charge per photoelectron
// still differs: a WCSim PE pulse has an area of ~22.7 mV x sample (5.8 mV peak
// x 7.76 ns / 2 ns, measured 22.7 with the truncated gain), a BGOg4Sim PE has
// 5 mV x sample (unit-area pulse x 5 mV). To put WCSim on the BGOg4Sim charge
// scale use ampScale = 0.001 / 4.54 = 2.2e-4 (the trigger threshold, given in mV
// of the stored waveform, is unchanged).
//
// Usage (inside the WCSim container, e.g. through tools/annie-run.sh):
//   root -l -b -q 'tagger_read_waveforms.C("ambe_gamma44_refsource_refpmt_0.root")'
//   root -l -b -q 'tagger_read_waveforms.C("run_*.root","refpmt",0.001,10.,true,4.44)'
//
// Arguments:
//   files            file name or wildcard (the *_lappd_* files are skipped)
//   outprefix        prefix of the output images and ROOT file
//   ampScale         factor applied to the mV samples (0.001: V)
//   triggerThreshold trigger threshold [mV of the stored waveform]
//   alignTrigger     shift the threshold crossing to 700 ns (BRF_* alignment)
//   feEnergy         if > 0, keep only events with |bgo_edep - feEnergy| <
//                    feHalfWidth [MeV] (BGOg4Sim has no such cut; use 0 for
//                    the same selection)
//   feHalfWidth      half width of the energy window [MeV]
//   seed             random seed for picking the example waveforms (0: from
//                    the clock, like read_waveforms.C)
//
// Waveform convention (AmBeTaggerWaveformBuilder): 1500 samples, 2 ns apart;
// sample k is bin k+1 of a TH1 with range [0, 3000] ns.

#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TH1D.h"
#include "TH2F.h"
#include "TRandom3.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
const double kDt = 2.;          // ns
const double kTriggerTime = 700.;  // ns

// SubtractBaseline() of read_waveforms.C (returns the baseline)
double ClippedBaseline(const std::vector<double>& w, int iterations)
{
  std::vector<double> values(w);
  for (int iter = 0; iter < iterations; ++iter) {
    double sum = 0.;
    for (double v : values) sum += v;
    const double mean = sum / values.size();
    double sq = 0.;
    for (double v : values) sq += (v - mean) * (v - mean);
    const double sigma = std::sqrt(sq / values.size());
    std::vector<double> clipped;
    for (double v : values) if (std::fabs(v - mean) < sigma) clipped.push_back(v);
    if (clipped.empty()) break;
    values.swap(clipped);
  }
  double baseline = 0.;
  for (double v : values) baseline += v;
  return baseline / values.size();
}

// Triggered, aligned, baseline-subtracted and scaled waveform; false if the
// event does not trigger.
bool Process(const std::vector<double>& raw, double ampScale, double threshold,
             bool align, std::vector<double>& out)
{
  const int n = raw.size();
  const double rawBaseline = ClippedBaseline(raw, 3);
  int kCross = -1;
  for (int k = 0; k < n; ++k)
    if (raw[k] - rawBaseline > threshold) { kCross = k; break; }
  if (kCross < 0) return false;

  std::vector<double> w(raw);
  if (align) {
    const int shift = static_cast<int>(kTriggerTime / kDt) - kCross;
    std::fill(w.begin(), w.end(), rawBaseline);
    for (int k = 0; k < n; ++k) {
      const int j = k + shift;
      if (j >= 0 && j < n) w[j] = raw[k];
    }
  }
  const double baseline = ClippedBaseline(w, 3);
  out.resize(n);
  for (int k = 0; k < n; ++k) out[k] = (w[k] - baseline) * ampScale;
  return true;
}

TH1D* MakeHist(const std::vector<double>& w, const char* name, const char* yTitle)
{
  TH1D* h = new TH1D(name, name, w.size(), 0., w.size() * kDt);
  for (size_t k = 0; k < w.size(); ++k) h->SetBinContent(k + 1, w[k]);
  h->GetXaxis()->SetTitle("Time [ns]");
  h->GetYaxis()->SetTitle(yTitle);
  return h;
}
}  // namespace

void tagger_read_waveforms(const char* files = "wcsim_0.root",
                           const char* outprefix = "tagger_read_waveforms",
                           double ampScale = 0.001, double triggerThreshold = 10.,
                           bool alignTrigger = true, double feEnergy = 0.,
                           double feHalfWidth = 0.01, unsigned seed = 0)
{
  gSystem->Load("libWCSimRoot.so");

  TChain all("wcsimT");
  all.Add(files);
  TChain chain("wcsimT");
  TIter next(all.GetListOfFiles());
  while (TObject* element = next()) {
    const TString name = element->GetTitle();
    if (!name.Contains("_lappd_")) chain.Add(name);
  }
  if (chain.GetNtrees() == 0) {
    std::cerr << "No WCSim output files match " << files << std::endl;
    return;
  }
  if (!chain.GetBranch("tagger_wf")) {
    std::cerr << "No tagger_wf branch: was the tagger enabled?" << std::endl;
    return;
  }

  chain.SetBranchStatus("*", 0);
  chain.SetBranchStatus("tagger_wf", 1);
  std::vector<double>* wf = 0;
  chain.SetBranchAddress("tagger_wf", &wf);
  double edep = 0.;
  if (feEnergy > 0.) {
    chain.SetBranchStatus("bgo_edep", 1);
    chain.SetBranchAddress("bgo_edep", &edep);
  }

  // Integration bins as FindBin() in read_waveforms.C (sample = bin - 1)
  const int kPromptStart = static_cast<int>(std::floor(690. / kDt));
  const int kPromptEnd = static_cast<int>(std::floor(850. / kDt));
  const int kFullEnd = static_cast<int>(std::floor(900. / kDt));
  const double fewThreshold = 30. * ampScale;  // ymax >= 0.03 V in read_waveforms.C

  std::vector<float> qTotal, fPrompt;
  std::vector<Long64_t> triggered, bright;  // entries, for the example plots
  std::vector<double> w;
  Long64_t nEmpty = 0, nNotFe = 0, nNoTrigger = 0, nBadQ = 0;
  const Long64_t nEntries = chain.GetEntries();

  for (Long64_t i = 0; i < nEntries; ++i) {
    chain.GetEntry(i);
    if (!wf || wf->empty()) { ++nEmpty; continue; }
    if (feEnergy > 0. && std::fabs(edep - feEnergy) >= feHalfWidth) { ++nNotFe; continue; }
    if (!Process(*wf, ampScale, triggerThreshold, alignTrigger, w)) { ++nNoTrigger; continue; }
    triggered.push_back(i);
    if (*std::max_element(w.begin(), w.end()) >= fewThreshold) bright.push_back(i);

    const int n = w.size();
    double qp = 0., qt = 0.;
    for (int k = std::max(0, kPromptStart); k <= std::min(n - 1, kFullEnd); ++k) {
      qt += w[k];
      if (k <= kPromptEnd) qp += w[k];
    }
    if (qt <= 0.) { ++nBadQ; continue; }
    qTotal.push_back(qt);
    fPrompt.push_back(qp / qt);
  }

  std::cout << "Files:                   " << files << "\n"
            << "Events:                  " << nEntries << "\n"
            << "Empty waveform (no PE):  " << nEmpty << "\n";
  if (feEnergy > 0.)
    std::cout << "Outside FE window:       " << nNotFe << " (bgo_edep = " << feEnergy
              << " +- " << feHalfWidth << " MeV)\n";
  std::cout << "Not triggered (< " << triggerThreshold << " mV): " << nNoTrigger << "\n"
            << "Total integral <= 0:     " << nBadQ << "\n"
            << "Entries in histogram:    " << qTotal.size() << std::endl;
  if (triggered.empty()) return;

  const char* yTitle = std::fabs(ampScale - 1.) < 1e-12 ? "Voltage [mV]"
                     : std::fabs(ampScale - 0.001) < 1e-12 ? "Voltage [V]"
                     : "Voltage [arb]";
  TRandom3 rng(seed);
  gStyle->SetOptStat(1111);

  // Single event
  TCanvas* singleEv = new TCanvas("Single Event", "Single Event", 1920, 1080);
  {
    const Long64_t entry = triggered[rng.Integer(triggered.size())];
    chain.GetEntry(entry);
    Process(*wf, ampScale, triggerThreshold, alignTrigger, w);
    MakeHist(w, TString::Format("WF_entry%lld", entry), yTitle)->Draw("hist");
  }
  singleEv->SaveAs(TString::Format("%s_single.png", outprefix));

  // Few events: random waveforms with maximum >= 30 mV, displayed to 2000 ns
  const int grid = 3;
  TCanvas* fewEv = new TCanvas("Few events", "Few events", 1920, 1080);
  fewEv->Divide(grid, grid);
  if (bright.empty()) std::cout << "no waveform above 30 mV" << std::endl;
  const std::vector<Long64_t>& pool = bright.empty() ? triggered : bright;
  for (int p = 0; p < grid * grid; ++p) {
    const Long64_t entry = pool[rng.Integer(pool.size())];
    chain.GetEntry(entry);
    Process(*wf, ampScale, triggerThreshold, alignTrigger, w);
    TH1D* h = MakeHist(w, TString::Format("WF_entry%lld_%d", entry, p), yTitle);
    h->GetXaxis()->SetRange(1, h->GetXaxis()->FindBin(2000.));
    fewEv->cd(p + 1);
    h->Draw("hist");
  }
  fewEv->SaveAs(TString::Format("%s_few.png", outprefix));

  // F_prompt vs charge, as in read_waveforms.C
  TH2F* h = new TH2F("h_fprompt_vs_energy",
                     "F_{prompt} vs Energy;Total Charge [arb];F_{prompt}",
                     200, -1, 8, 120, 0.0, 1.2);
  for (size_t j = 0; j < qTotal.size(); ++j) h->Fill(qTotal[j], fPrompt[j]);
  TCanvas* c3 = new TCanvas("c3", "Fprompt vs Energy", 800, 600);
  h->Draw("COLZ");
  c3->SaveAs(TString::Format("%s_fprompt.png", outprefix));

  std::cout << "Charge: mean " << h->GetMean(1) << ", RMS " << h->GetStdDev(1)
            << "; F_prompt: mean " << h->GetMean(2) << ", RMS " << h->GetStdDev(2)
            << " (under/overflow in charge: "
            << h->ProjectionX()->GetBinContent(0) << " / "
            << h->ProjectionX()->GetBinContent(201) << ")" << std::endl;

  TFile out(TString::Format("%s.root", outprefix), "RECREATE");
  h->Write();
  out.Close();
}
