// Plots the AmBe tagger PMT readout stored in wcsimT: photoelectron count,
// waveform observables and example waveforms of full-energy events. The
// branches only exist when WCSim ran with /WCSim/AmBe/IncludeAmBeTagger true.
//
// Usage (inside the WCSim container, e.g. through tools/annie-run.sh):
//   root -l -b -q 'plot_tagger_waveform.C("g44_*.root")'
//   root -l -b -q 'plot_tagger_waveform.C("run.root", "tagger.png", 4.4, 0.01, 9, 600., 2200.)'
//   global time instead of waveform time (default window):
//   root -l -b -q 'plot_tagger_waveform.C("run.root", "tagger.png", 4.4, 0.01, 6, 0., 0., 45.0, 0, true)'
//
// Arguments:
//   files        file name or wildcard; all files are chained (the *_lappd_*
//                files WCSim writes alongside are skipped).
//   outname      base name of the images. Three are written:
//                  <outname>             summary (6 pads, see below)
//                  <outname>_timing      peak-time distribution and the mean
//                                        full-energy waveform
//                  <outname>_examples    example full-energy waveforms
//   gammaEnergy  full-energy line [MeV]; events with |bgo_edep - gammaEnergy|
//                < feHalfWidth count as full-energy (FE) events.
//   feHalfWidth  [MeV]
//   nExamples    number of example waveforms (first FE events in the chain,
//                after skipping exampleSkip of them)
//   tmin, tmax   time range of the example and mean waveforms and of the
//                peak-time histograms [ns], in the time frame chosen by
//                globalTime. tmax <= tmin selects the default window:
//                600-2200 ns waveform time = -100-1500 ns global time.
//   peArea       area of one average photoelectron pulse [mV*ns], used to turn
//                tagger_wf_integral into photoelectrons. Default: 5.8 mV gain x
//                7.76 ns area per unit peak of the normalised pulse.
//   exampleSkip  FE events to skip before taking the examples
//   globalTime   false (default): plot in waveform time, t = k*dt, measured
//                from sample 0 (0-2998 ns). true: plot in global time,
//                t = t0 + k*dt. tagger_wf_peak_time (stored in global time)
//                is converted accordingly.
//
// Waveform convention (WCSimEventAction / AmBeTaggerWaveformBuilder): 1500
// samples, 2 ns apart, sample k at global time t0 + k*dt with t0 = -700 ns,
// i.e. waveform time = global time - t0. tagger_wf_peak_time is global time.
// Events without photoelectrons store an empty waveform and zero observables.
//
// Summary pads:
//   1 tagger_npe, all events with PE and FE events
//   2 tagger_wf_integral / peArea (charge in PE units), all and FE
//   3 tagger_npe vs bgo_edep
//   4 tagger_wf_integral / peArea vs tagger_npe (dashed: y = x)
//   5 tagger_wf_peak, all and FE
//   6 tagger_wf_peak vs tagger_npe

#include "TCanvas.h"
#include "TChain.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMarker.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
// <outname> with "_<tag>" inserted before the extension
TString TaggedName(const char* outname, const char* tag)
{
  TString name(outname);
  const Ssiz_t dot = name.Last('.');
  if (dot == kNPOS) name += TString::Format("_%s.png", tag);
  else name.Insert(dot, TString::Format("_%s", tag));
  return name;
}

void StyleHist(TH1* h, int color, bool filled)
{
  h->SetLineColor(color);
  h->SetLineWidth(2);
  if (filled) h->SetFillColorAlpha(color, 0.3);
  h->SetStats(false);
}
}  // namespace

void plot_tagger_waveform(const char* files = "wcsim_0.root",
                          const char* outname = "tagger_waveform.png",
                          double gammaEnergy = 4.4, double feHalfWidth = 0.01,
                          int nExamples = 6, double tmin = 0., double tmax = 0.,
                          double peArea = 5.8 * 7.763, int exampleSkip = 0,
                          bool globalTime = false)
{
  gSystem->Load("libWCSimRoot.so");

  const double t0 = -700.;  // ns, time of sample 0
  const double dt = 2.;     // ns, sample spacing

  // Time frame of the plots: sample k is drawn at tOffset + k*dt, and a global
  // time tg at tg - t0 + tOffset.
  const double tOffset = globalTime ? t0 : 0.;
  const char* timeLabel = globalTime ? "Global time [ns]" : "Waveform time [ns]";
  if (tmax <= tmin) {
    tmin = -100. - t0 + tOffset;
    tmax = 1500. - t0 + tOffset;
  }

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
  const char* needed[] = {"bgo_edep", "tagger_npe", "tagger_wf_peak",
                          "tagger_wf_peak_time", "tagger_wf_integral", "tagger_wf"};
  for (const char* b : needed) {
    if (!chain.GetBranch(b)) {
      std::cerr << "No " << b << " branch: was the tagger enabled, and is this "
                << "output from a WCSim build with the waveform branches?" << std::endl;
      return;
    }
  }

  chain.SetBranchStatus("*", 0);
  for (const char* b : needed) chain.SetBranchStatus(b, 1);
  double edep = 0., peak = 0., peakTime = 0., integral = 0.;
  int npe = 0;
  std::vector<double>* wf = 0;
  chain.SetBranchAddress("bgo_edep", &edep);
  chain.SetBranchAddress("tagger_npe", &npe);
  chain.SetBranchAddress("tagger_wf_peak", &peak);
  chain.SetBranchAddress("tagger_wf_peak_time", &peakTime);
  chain.SetBranchAddress("tagger_wf_integral", &integral);
  chain.SetBranchAddress("tagger_wf", &wf);

  // First pass: keep the scalars (cheap), sum the FE waveforms and copy the
  // example ones. Histogram ranges are set afterwards from the maxima.
  struct Event { double edep, peak, peakTime, qpe; int npe; bool fe; };
  std::vector<Event> events;
  std::vector<double> meanWf;
  std::vector<std::vector<double> > examples;
  std::vector<Event> exampleInfo;
  std::vector<Long64_t> exampleEntry;
  Long64_t nFE = 0, nFEwithWf = 0, nNoPE = 0;
  const Long64_t nEntries = chain.GetEntries();
  for (Long64_t i = 0; i < nEntries; ++i) {
    chain.GetEntry(i);
    const bool fe = std::fabs(edep - gammaEnergy) < feHalfWidth;
    if (fe) ++nFE;
    if (npe <= 0) { ++nNoPE; continue; }
    Event ev = {edep, peak, peakTime - t0 + tOffset, integral / peArea, npe, fe};
    events.push_back(ev);
    if (!fe || !wf || wf->empty()) continue;
    ++nFEwithWf;
    if (meanWf.empty()) meanWf.assign(wf->size(), 0.);
    for (size_t k = 0; k < wf->size() && k < meanWf.size(); ++k) meanWf[k] += (*wf)[k];
    if (nFEwithWf > exampleSkip && (int)examples.size() < nExamples) {
      examples.push_back(*wf);
      exampleInfo.push_back(ev);
      exampleEntry.push_back(i);
    }
  }
  if (events.empty()) {
    std::cerr << "No events with photoelectrons in " << files << std::endl;
    return;
  }

  int maxNpe = 1;
  double maxPeak = 1., maxEdep = gammaEnergy;
  for (const Event& e : events) {
    maxNpe = std::max(maxNpe, e.npe);
    maxPeak = std::max(maxPeak, e.peak);
    maxEdep = std::max(maxEdep, e.edep);
  }
  const double npeMax = 1.15 * maxNpe;
  const double peakMax = 1.15 * maxPeak;
  const int nbNpe = std::min(100, std::max(20, maxNpe / 3));

  TH1D* hNpe = new TH1D("h_npe", "Photoelectrons;tagger_npe;Events", nbNpe, 0., npeMax);
  TH1D* hNpeFE = new TH1D("h_npe_fe", "", nbNpe, 0., npeMax);
  TH1D* hQ = new TH1D("h_q", "Waveform charge;tagger_wf_integral / peArea [PE];Events",
                      nbNpe, -5., npeMax);
  TH1D* hQFE = new TH1D("h_q_fe", "", nbNpe, -5., npeMax);
  TH2D* hNpeEdep = new TH2D("h_npe_edep", "PE vs BGO deposit;bgo_edep [MeV];tagger_npe",
                            60, 0., 1.05 * maxEdep, 60, 0., npeMax);
  TH2D* hQNpe = new TH2D("h_q_npe", "Charge vs PE count;tagger_npe;tagger_wf_integral / peArea [PE]",
                         60, 0., npeMax, 60, 0., npeMax);
  TH1D* hPeak = new TH1D("h_peak", "Waveform peak;tagger_wf_peak [mV];Events", 60, 0., peakMax);
  TH1D* hPeakFE = new TH1D("h_peak_fe", "", 60, 0., peakMax);
  TH2D* hPeakNpe = new TH2D("h_peak_npe", "Peak vs PE count;tagger_npe;tagger_wf_peak [mV]",
                            60, 0., npeMax, 60, 0., peakMax);
  TH1D* hPeakTime = new TH1D("h_peak_time",
                             TString::Format("Waveform peak time;tagger_wf_peak_time, %s;Events",
                                             timeLabel),
                             80, tmin, tmax);
  TH1D* hPeakTimeFE = new TH1D("h_peak_time_fe", "", 80, tmin, tmax);

  double sumNpeFE = 0., sumNpe2FE = 0., sumQFE = 0., sumQ2FE = 0.;
  double sumPeakTimeFE = 0.;
  for (const Event& e : events) {
    hNpe->Fill(e.npe);
    hQ->Fill(e.qpe);
    hNpeEdep->Fill(e.edep, e.npe);
    hQNpe->Fill(e.npe, e.qpe);
    hPeak->Fill(e.peak);
    hPeakNpe->Fill(e.npe, e.peak);
    hPeakTime->Fill(e.peakTime);
    if (!e.fe) continue;
    hNpeFE->Fill(e.npe);
    hQFE->Fill(e.qpe);
    hPeakFE->Fill(e.peak);
    hPeakTimeFE->Fill(e.peakTime);
    sumNpeFE += e.npe;  sumNpe2FE += double(e.npe) * e.npe;
    sumQFE += e.qpe;    sumQ2FE += e.qpe * e.qpe;
    sumPeakTimeFE += e.peakTime;
  }

  // Printout
  std::cout << "Files:                    " << files << "\n"
            << "Events:                   " << nEntries << "\n"
            << "Events without PE:        " << nNoPE << "\n"
            << "Full-energy events (+-" << feHalfWidth * 1000 << " keV around "
            << gammaEnergy << " MeV): " << nFE << " (" << nFEwithWf
            << " with a waveform)" << std::endl;
  const Long64_t nFEpe = (Long64_t)hNpeFE->GetEntries();
  if (nFEpe > 0) {
    const double mN = sumNpeFE / nFEpe, sN = std::sqrt(std::max(0., sumNpe2FE / nFEpe - mN * mN));
    const double mQ = sumQFE / nFEpe, sQ = std::sqrt(std::max(0., sumQ2FE / nFEpe - mQ * mQ));
    std::cout << "FE tagger_npe:            mean " << mN << ", sigma/mean " << sN / mN
              << " (Poisson alone " << 1. / std::sqrt(mN) << ")\n"
              << "FE charge [PE]:           mean " << mQ << ", sigma/mean " << sQ / mQ
              << "  (charge/npe " << mQ / mN << ")\n"
              << "FE PE per MeV:            " << mN / gammaEnergy << "\n"
              << "FE mean peak time:        " << sumPeakTimeFE / nFEpe << " ns ("
            << (globalTime ? "global" : "waveform") << " time)" << std::endl;
  } else {
    std::cout << "No full-energy events with photoelectrons." << std::endl;
  }

  // ---- Summary canvas
  gStyle->SetOptStat(0);
  TCanvas* c = new TCanvas("c_tagger_summary", "Tagger PMT readout", 1500, 900);
  c->Divide(3, 2);

  const int cAll = kAzure + 2, cFE = kRed + 1;
  StyleHist(hNpe, cAll, true);    StyleHist(hNpeFE, cFE, false);
  StyleHist(hQ, cAll, true);      StyleHist(hQFE, cFE, false);
  StyleHist(hPeak, cAll, true);   StyleHist(hPeakFE, cFE, false);
  StyleHist(hPeakTime, cAll, true); StyleHist(hPeakTimeFE, cFE, false);

  TLegend* leg = new TLegend(0.55, 0.75, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->AddEntry(hNpe, "events with PE", "f");
  leg->AddEntry(hNpeFE, TString::Format("full energy (%.2f MeV)", gammaEnergy), "l");

  c->cd(1); hNpe->Draw("hist"); hNpeFE->Draw("hist same"); leg->Draw();
  c->cd(2); hQ->Draw("hist");   hQFE->Draw("hist same");   leg->Draw();
  c->cd(3); gPad->SetRightMargin(0.13); hNpeEdep->SetStats(false); hNpeEdep->Draw("colz");
  c->cd(4); gPad->SetRightMargin(0.13); hQNpe->SetStats(false); hQNpe->Draw("colz");
  TLine* diag = new TLine(0., 0., npeMax, npeMax);
  diag->SetLineStyle(2); diag->SetLineColor(kRed + 1); diag->Draw();
  c->cd(5); hPeak->Draw("hist"); hPeakFE->Draw("hist same"); leg->Draw();
  c->cd(6); gPad->SetRightMargin(0.13); hPeakNpe->SetStats(false); hPeakNpe->Draw("colz");
  c->SaveAs(outname);

  // ---- Timing canvas: peak-time distribution and mean FE waveform
  TCanvas* ct = new TCanvas("c_tagger_timing", "Tagger PMT timing", 1400, 600);
  ct->Divide(2, 1);
  ct->cd(1);
  hPeakTime->Draw("hist"); hPeakTimeFE->Draw("hist same"); leg->Draw();
  ct->cd(2);
  if (nFEwithWf > 0) {
    TGraph* gMean = new TGraph();
    for (size_t k = 0; k < meanWf.size(); ++k) {
      const double t = tOffset + k * dt;
      if (t < tmin || t > tmax) continue;
      gMean->SetPoint(gMean->GetN(), t, meanWf[k] / nFEwithWf);
    }
    gMean->SetTitle(TString::Format("Mean full-energy waveform (%lld events);"
                                    "%s;Amplitude [mV]", nFEwithWf, timeLabel));
    gMean->SetLineColor(cFE);
    gMean->SetLineWidth(2);
    gMean->Draw("AL");
  }
  ct->SaveAs(TaggedName(outname, "timing"));

  // ---- Example FE waveforms
  if (examples.empty()) {
    std::cout << "No full-energy waveforms to show as examples." << std::endl;
    return;
  }
  const int nEx = examples.size();
  const int nCols = nEx <= 2 ? nEx : (nEx <= 4 ? 2 : 3);
  const int nRows = (nEx + nCols - 1) / nCols;
  TCanvas* ce = new TCanvas("c_tagger_examples", "Tagger example waveforms",
                            500 * nCols, 380 * nRows);
  ce->Divide(nCols, nRows);
  TLatex label;
  label.SetNDC();
  label.SetTextSize(0.045);
  for (int j = 0; j < nEx; ++j) {
    ce->cd(j + 1);
    TGraph* g = new TGraph();
    for (size_t k = 0; k < examples[j].size(); ++k) {
      const double t = tOffset + k * dt;
      if (t < tmin || t > tmax) continue;
      g->SetPoint(g->GetN(), t, examples[j][k]);
    }
    g->SetTitle(TString::Format("Entry %lld;%s;Amplitude [mV]", exampleEntry[j], timeLabel));
    g->SetLineColor(cAll);
    g->Draw("AL");
    const Event& e = exampleInfo[j];
    TMarker* m = new TMarker(e.peakTime, e.peak, 23);
    m->SetMarkerColor(cFE);
    m->SetMarkerSize(1.4);
    m->Draw();
    label.DrawLatex(0.50, 0.82, TString::Format("E_{dep} = %.3f MeV", e.edep));
    label.DrawLatex(0.50, 0.76, TString::Format("N_{PE} = %d, Q = %.1f PE", e.npe, e.qpe));
    label.DrawLatex(0.50, 0.70, TString::Format("peak %.1f mV at %.0f ns", e.peak, e.peakTime));
  }
  ce->SaveAs(TaggedName(outname, "examples"));
}
