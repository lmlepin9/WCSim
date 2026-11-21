// Plots the AmBe tagger BGO energy-deposit spectrum (branch "bgo_edep" of
// wcsimT, in MeV, one entry per event). The branch only exists when WCSim ran
// with /WCSim/AmBe/IncludeAmBeTagger true.
//
// Usage (inside the WCSim container, e.g. through tools/annie-run.sh):
//   root -l -b -q 'plot_bgo_edep.C("ambe_gamma44_iso_centre_*.root")'
//   root -l -b -q 'plot_bgo_edep.C("run.root", 230, -0.01, 4.59, "spectrum.png", true, true)'
//
// Arguments:
//   files      file name or wildcard. All files are chained, so output split
//              over several files (WCSim starts a new one every 10000 events)
//              can be read together.
//   nbins, emin, emax   histogram binning [MeV]. The default range is shifted
//              by half a bin (20 keV bins, edges at 4.39 and 4.41) so that the
//              full-energy line sits mid-bin. With an edge exactly at the line,
//              rounding in the summed step deposits splits the peak in two bins.
//   outname    output image (any format TCanvas::SaveAs understands)
//   skipZero   leave out events with no BGO deposit. Their number is still
//              printed. Otherwise they make a large spike in the first bin.
//   logy       log scale on the y axis
//
// Dashed lines mark the 4.44 MeV 12C* line (Geant4 gun energy 4.4 MeV) and its
// single- and double-escape peaks, at E, E - 0.511 and E - 1.022 MeV.
//
// If the file also has the BGO optical-photon branches (bgo_nscint_created,
// bgo_ncher_created, bgo_nphot_tracked), a second image <outname>_photons.<ext>
// is made, for events with at least one photon created in the BGO:
//   left:  scintillation photons created vs energy deposit. The dashed line is
//          the nominal BGO light yield (lightYield, default 10000 / MeV).
//   right: tracked / created photons. This checks the BGO exemption in
//          WCSimStackingAction: 1 when no BGO photon is culled; about the tank
//          PMT QE (~0.25-0.3) when the tank QE culling is applied.
// If bgo_evis (Birks-quenched energy) exists, the scintillation yield is also
// printed per visible MeV: it should equal lightYield when G4Scintillation
// applies Birks saturation, and lightYield per deposited MeV when it does not.

#include "TCanvas.h"
#include "TChain.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLine.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>

void plot_bgo_edep(const char* files = "wcsim_0.root",
                   int nbins = 230, double emin = -0.01, double emax = 4.59,
                   const char* outname = "bgo_edep.png",
                   bool skipZero = true, bool logy = false,
                   double gammaEnergy = 4.4, double lightYield = 10000.)
{
  // Not needed to read a plain double, but avoids "no dictionary" warnings
  // for the WCSimRootEvent branches in the same tree.
  gSystem->Load("libWCSimRoot.so");

  // Expand the wildcard, then drop the *_lappd_N.root files WCSim writes next
  // to the main output: they share the name prefix but have no wcsimT tree.
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
  if (!chain.GetBranch("bgo_edep")) {
    std::cerr << "No bgo_edep branch: was the tagger enabled in jobOptions.mac?"
              << std::endl;
    return;
  }

  // Optional photon-counter branches (added after bgo_edep; older files lack them)
  const bool hasPhotons = chain.GetBranch("bgo_nscint_created") &&
                          chain.GetBranch("bgo_ncher_created") &&
                          chain.GetBranch("bgo_nphot_tracked");

  // Read only the branches we need; the event branches are large.
  chain.SetBranchStatus("*", 0);
  chain.SetBranchStatus("bgo_edep", 1);
  double edep = 0.;
  chain.SetBranchAddress("bgo_edep", &edep);
  int nScint = 0, nCher = 0, nTracked = 0;
  double evis = 0.;
  const bool hasEvis = chain.GetBranch("bgo_evis") != 0;
  if (hasEvis) {
    chain.SetBranchStatus("bgo_evis", 1);
    chain.SetBranchAddress("bgo_evis", &evis);
  }
  if (hasPhotons) {
    chain.SetBranchStatus("bgo_nscint_created", 1);
    chain.SetBranchStatus("bgo_ncher_created", 1);
    chain.SetBranchStatus("bgo_nphot_tracked", 1);
    chain.SetBranchAddress("bgo_nscint_created", &nScint);
    chain.SetBranchAddress("bgo_ncher_created", &nCher);
    chain.SetBranchAddress("bgo_nphot_tracked", &nTracked);
  }

  TH1D* h = new TH1D("h_bgo_edep",
                     ";Energy deposited in BGO [MeV];Events",
                     nbins, emin, emax);

  // Photon histograms. The scintillation axis covers the light yield times
  // the energy range, with headroom for the resolution spread.
  const double maxScint = 1.5 * lightYield * emax;
  TH2D* hScint = new TH2D("h_bgo_nscint",
                          "Scintillation photons created in BGO;"
                          "Energy deposited in BGO [MeV];Scintillation photons",
                          nbins / 2, emin, emax, 100, 0., maxScint);
  TH1D* hRatio = new TH1D("h_bgo_tracked_ratio",
                          "BGO photons surviving the stacking action;"
                          "Tracked / created photons;Events",
                          110, -0.005, 1.095);  // 1.0 sits mid-bin
  double sumScint = 0., sumEdepPhot = 0., sumEvisPhot = 0.;
  double ratioMin = 1e9, ratioMax = -1e9;
  Long64_t nPhotEvents = 0, nOverOne = 0;
  long long sumCreated = 0, sumTracked = 0, sumCher = 0;

  const Long64_t nEntries = chain.GetEntries();
  Long64_t nZero = 0, nPeak = 0;
  const double peakHalfWidth = 0.01;  // MeV, for the full-energy count
  for (Long64_t i = 0; i < nEntries; ++i) {
    chain.GetEntry(i);
    const int nCreated = nScint + nCher;
    if (hasPhotons && nCreated > 0) {
      ++nPhotEvents;
      sumCreated += nCreated;
      sumTracked += nTracked;
      sumCher += nCher;
      sumScint += nScint;
      sumEdepPhot += edep;
      sumEvisPhot += evis;
      const double ratio = double(nTracked) / nCreated;
      ratioMin = std::min(ratioMin, ratio);
      ratioMax = std::max(ratioMax, ratio);
      if (nTracked > nCreated) ++nOverOne;
      hScint->Fill(edep, nScint);
      hRatio->Fill(ratio);
    }
    if (edep <= 0.) {
      ++nZero;
      if (skipZero) continue;
    }
    if (std::fabs(edep - gammaEnergy) < peakHalfWidth) ++nPeak;
    h->Fill(edep);
  }

  std::cout << "Files:                  " << files << "\n"
            << "Events:                 " << nEntries << "\n"
            << "No BGO deposit:         " << nZero << " ("
            << (nEntries ? 100. * nZero / nEntries : 0.) << " %)\n"
            << "Full energy (+-" << peakHalfWidth * 1000 << " keV): " << nPeak
            << " (" << (nEntries ? 100. * nPeak / nEntries : 0.) << " %)"
            << std::endl;
  if (hasPhotons && nPhotEvents > 0) {
    std::cout << "Events with BGO photons: " << nPhotEvents << "\n"
              << "Created photons:        " << sumCreated << " ("
              << sumCher << " Cherenkov)\n"
              << "Tracked / created:      " << double(sumTracked) / sumCreated
              << " overall, per event min " << ratioMin << " max " << ratioMax
              << "\n"
              << "Events tracked > created: " << nOverOne << "\n"
              << "Scintillation / MeV:    "
              << (sumEdepPhot > 0. ? sumScint / sumEdepPhot : 0.)
              << " (nominal " << lightYield << ")" << std::endl;
    if (hasEvis && sumEdepPhot > 0. && sumEvisPhot > 0.) {
      std::cout << "Visible / deposited E:  " << sumEvisPhot / sumEdepPhot << "\n"
                << "Scintillation / vis MeV: " << sumScint / sumEvisPhot
                << " (nominal " << lightYield << ")" << std::endl;
    }
  } else if (!hasPhotons) {
    std::cout << "No BGO photon branches: photon plots skipped." << std::endl;
  }

  gStyle->SetOptStat(1110);  // entries, mean, RMS
  // Stats box top left: the peaks sit at the high-energy end.
  gStyle->SetStatX(0.40);
  gStyle->SetStatY(0.88);
  TCanvas* c = new TCanvas("c_bgo_edep", "BGO energy deposit", 900, 600);
  c->SetLogy(logy);
  h->SetLineColor(kAzure + 2);
  h->SetFillColorAlpha(kAzure + 2, 0.25);
  h->SetTitle(skipZero ? "BGO energy deposit (events with E_{dep} > 0)"
                       : "BGO energy deposit");
  if (logy) h->SetMinimum(0.5);
  h->Draw("hist");
  c->Update();

  // Reference lines: full energy, single escape, double escape.
  const double lines[3] = {gammaEnergy, gammaEnergy - 0.511, gammaEnergy - 1.022};
  const char* labels[3] = {"FE", "SE", "DE"};
  TLatex text;
  text.SetTextSize(0.03);
  text.SetTextColor(kRed + 1);
  text.SetTextAlign(21);
  // With a log y axis the pad range is stored as log10.
  const double ymin = logy ? std::pow(10., c->GetUymin()) : c->GetUymin();
  const double ymax = logy ? std::pow(10., c->GetUymax()) : c->GetUymax();
  const double ytext = logy ? std::pow(10., c->GetUymin() + 0.93 * (c->GetUymax() - c->GetUymin()))
                            : ymin + 0.93 * (ymax - ymin);
  for (int i = 0; i < 3; ++i) {
    if (lines[i] < emin || lines[i] > emax) continue;
    TLine* l = new TLine(lines[i], ymin, lines[i], ymax);
    l->SetLineStyle(2);
    l->SetLineColor(kRed + 1);
    l->Draw();
    text.DrawLatex(lines[i], ytext, labels[i]);
  }

  c->SaveAs(outname);

  if (!hasPhotons || nPhotEvents == 0) return;

  // Second image: <outname>_photons.<ext>
  TString photonName(outname);
  const Ssiz_t dot = photonName.Last('.');
  if (dot == kNPOS) photonName += "_photons.png";
  else photonName.Insert(dot, "_photons");

  TCanvas* cp = new TCanvas("c_bgo_photons", "BGO optical photons", 1400, 600);
  cp->Divide(2, 1);

  cp->cd(1);
  gPad->SetRightMargin(0.13);
  gPad->SetLeftMargin(0.14);
  hScint->GetYaxis()->SetTitleOffset(1.6);
  hScint->SetStats(false);
  hScint->Draw("colz");
  TLine* yieldLine = new TLine(std::max(emin, 0.), lightYield * std::max(emin, 0.),
                               std::min(emax, maxScint / lightYield),
                               lightYield * std::min(emax, maxScint / lightYield));
  yieldLine->SetLineStyle(2);
  yieldLine->SetLineColor(kRed + 1);
  yieldLine->SetLineWidth(2);
  yieldLine->Draw();

  cp->cd(2);
  hRatio->SetStats(false);
  hRatio->SetLineColor(kAzure + 2);
  hRatio->SetFillColorAlpha(kAzure + 2, 0.25);
  hRatio->Draw("hist");

  cp->SaveAs(photonName);
}
