// Pulse-shape plot for the AmBe tagger PMT: 2D histogram of fprompt vs the
// integrated charge, computed from the stored waveforms (tagger_wf). The
// branch only exists when WCSim ran with /WCSim/AmBe/IncludeAmBeTagger true.
//
// fprompt and the charge are computed as in the data macro read_waveforms.C,
// so simulation and data can be compared directly:
//   baseline  iterative sigma clipping over the whole waveform: keep the
//             samples with |v - mean| < sigma, repeat baselineIter times, and
//             take the mean of what is left; it is subtracted from every sample
//   windows   fixed, in waveform time [ns]: prompt [promptStart, promptEnd],
//             total [promptStart, totalEnd]
//   integral  plain sum of the baseline-subtracted samples whose bin contains
//             a window edge or lies between them (both ends inclusive, like
//             TH1::Integral(FindBin(t1), FindBin(t2))); not multiplied by dt
//   fprompt   prompt integral / total integral; events with total <= 0 are
//             skipped
//   cut       events whose baseline-subtracted maximum (over the whole
//             waveform, like GetMaximum()) is below peakThreshold are skipped,
//             as for the reference data plot
// The charge is in units of ampScale*mV*sample: ampScale = 1 keeps mV, 0.001
// gives V (if the data waveforms are in V).
//
// Usage (inside the WCSim container, e.g. through tools/annie-run.sh):
//   root -l -b -q 'tagger_fprompt_vs_charge.C("g44_*.root")'
//   root -l -b -q 'tagger_fprompt_vs_charge.C("run.root", "fprompt.png", 690., 850., 900.)'
//
// Arguments:
//   files         file name or wildcard; all files are chained (the *_lappd_*
//                 files WCSim writes alongside are skipped).
//   outname       image name; the histogram is also written to <outname
//                 without extension>.root
//   promptStart   start of both windows [ns, waveform time]
//   promptEnd     end of the prompt window [ns]
//   totalEnd      end of the total window [ns]
//   baselineIter  sigma-clipping iterations (3 in read_waveforms.C)
//   ampScale      amplitude scale factor applied to the mV samples
//   peakThreshold minimum waveform maximum after baseline subtraction [mV,
//                 independent of ampScale]; <= 0 disables the cut
//
// Waveform convention (WCSimEventAction / AmBeTaggerWaveformBuilder): 1500
// samples, 2 ns apart; sample k covers waveform time [k*dt, (k+1)*dt), i.e.
// it is bin k+1 of a TH1 with range [0, 3000] ns. Global time = waveform
// time - 700 ns. Events without photoelectrons store an empty waveform and
// are skipped.

#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TH2D.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
// Baseline as in SubtractBaseline() of read_waveforms.C
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
}  // namespace

void tagger_fprompt_vs_charge(const char* files = "wcsim_0.root",
                              const char* outname = "tagger_fprompt_vs_charge.png",
                              double promptStart = 690., double promptEnd = 850.,
                              double totalEnd = 900., int baselineIter = 3,
                              double ampScale = 1., double peakThreshold = 10.)
{
  gSystem->Load("libWCSimRoot.so");

  const double dt = 2.;  // ns, sample spacing

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

  // Sample containing time t (= TH1 bin FindBin(t) - 1)
  const int kStart = (int)std::floor(promptStart / dt);
  const int kPromptEnd = (int)std::floor(promptEnd / dt);
  const int kTotalEnd = (int)std::floor(totalEnd / dt);

  std::vector<double> qTot, fPrompt;
  Long64_t nEmpty = 0, nBelowPeak = 0, nBadQ = 0;
  const Long64_t nEntries = chain.GetEntries();
  for (Long64_t i = 0; i < nEntries; ++i) {
    chain.GetEntry(i);
    if (!wf || wf->empty()) { ++nEmpty; continue; }
    const std::vector<double>& w = *wf;
    const int n = w.size();
    const double baseline = ClippedBaseline(w, baselineIter);
    if (peakThreshold > 0. &&
        *std::max_element(w.begin(), w.end()) - baseline < peakThreshold) {
      ++nBelowPeak;
      continue;
    }

    double qp = 0., qt = 0.;
    for (int k = std::max(0, kStart); k <= std::min(n - 1, kTotalEnd); ++k) {
      const double v = (w[k] - baseline) * ampScale;
      qt += v;
      if (k <= kPromptEnd) qp += v;
    }
    if (qt <= 0.) { ++nBadQ; continue; }
    qTot.push_back(qt);
    fPrompt.push_back(qp / qt);
  }

  std::cout << "Files:                   " << files << "\n"
            << "Events:                  " << nEntries << "\n"
            << "Empty waveform (no PE):  " << nEmpty << "\n"
            << "Maximum < " << peakThreshold << " mV:        " << nBelowPeak << "\n"
            << "Total integral <= 0:     " << nBadQ << "\n"
            << "Entries in histogram:    " << qTot.size() << "\n"
            << "Windows [ns]: prompt " << promptStart << "-" << promptEnd
            << ", total " << promptStart << "-" << totalEnd << std::endl;
  if (qTot.empty()) return;

  // fprompt range as in read_waveforms.C; charge range from the maximum, since
  // the units depend on ampScale.
  const double qMax = *std::max_element(qTot.begin(), qTot.end());
  const double fMin = 0., fMax = 1.2;
  Long64_t nOut = 0;
  for (double f : fPrompt) if (f < fMin || f >= fMax) ++nOut;
  std::cout << "fprompt outside [" << fMin << ", " << fMax << "): " << nOut
            << " (kept in the under/overflow bins)" << std::endl;

  TH2D* h = new TH2D("h_fprompt_q",
                     TString::Format("F_{prompt} vs charge (prompt %.0f-%.0f ns, total %.0f-%.0f ns);"
                                     "Total charge [arb];F_{prompt}",
                                     promptStart, promptEnd, promptStart, totalEnd),
                     200, 0., 1.05 * qMax, 120, fMin, fMax);
  for (size_t j = 0; j < qTot.size(); ++j) h->Fill(qTot[j], fPrompt[j]);

  gStyle->SetOptStat(0);
  TCanvas* c = new TCanvas("c_fprompt_q", "fprompt vs charge", 900, 700);
  c->SetRightMargin(0.13);
  //c->SetLogz();
  h->Draw("colz");
  c->SaveAs(outname);

  TString rootName(outname);
  const Ssiz_t dot = rootName.Last('.');
  if (dot != kNPOS) rootName.Remove(dot);
  rootName += ".root";
  TFile out(rootName, "RECREATE");
  h->Write();
  out.Close();
  std::cout << "Histogram written to " << rootName << std::endl;
}
