#include "TFile.h"
#include "TDatime.h"
#include "TNamed.h"
#include "TMath.h"
#include "TLorentzVector.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TGraphAsymmErrors.h"
#include "TSystem.h"

#include "TopLJets2015/TopAnalysis/interface/BtagUncertaintyComputer.h"
#include "TopLJets2015/TopAnalysis/interface/Run5TeVAnalysis.h"
#include "TopLJets2015/TopAnalysis/interface/TOP-16-006.h"

#include <string>
#include <vector>

using namespace std;

//
void Run5TeVAnalysis(TString inFileName,
		     TString outFileName,
		     Int_t channelSelection,
		     Int_t chargeSelection,
                     FlavourSplitting flavourSplitting,
                     TH1F *normH,
                     Bool_t runSysts,
		     TString era)
{
  if(inFileName=="") 
    {
      std::cout << "No inputs specified. return" << std::endl;
      return;
    }

  bool isMC(false);
  if(inFileName.Contains("/MC")) isMC=true;
  bool isTTJets(false);
  if(inFileName.Contains("/MCTTNominal")) isTTJets=true;

  float totalEvtNorm(1.0);
  if(isMC && normH) totalEvtNorm=normH->GetBinContent(1);
  if(!isMC)
    {
      if( (channelSelection==11 || channelSelection==1100) && inFileName.Contains("FilteredSingleMuHighPt")) return;
      if( (channelSelection==13 || channelSelection==1300) && inFileName.Contains("HighPtLowerPhotons"))     return;
      runSysts=false;
    }
  std::cout << "Will process " << inFileName << " and save the results in " << outFileName << endl
	    << "Sample will be treated as MC=" << isMC <<  std::endl
	    << "Systematics will be run=" << runSysts << std::endl
	    << "Corrections to be retrieved from era=" << era << std::endl
	    << "Total normalization factor=" << totalEvtNorm << std::endl;

  std::map<TString, TGraphAsymmErrors *> expBtagEff;
  BTagSFUtil myBTagSFUtil;
  if(isMC)
    { 
      TString btagEffExpUrl(era+"/expTageff.root");
      gSystem->ExpandPathName(era+"/expTageff.root");   
      TFile *beffIn=TFile::Open(btagEffExpUrl);
      expBtagEff["b"]=(TGraphAsymmErrors *)beffIn->Get("b");
      expBtagEff["c"]=(TGraphAsymmErrors *)beffIn->Get("c");
      expBtagEff["udsg"]=(TGraphAsymmErrors *)beffIn->Get("udsg");
      beffIn->Close();
      cout << "Read " << expBtagEff.size() << " b-tag efficiency expectations from " << btagEffExpUrl << endl;
    }

  std::vector<TString>* inFileNames_p = new std::vector<TString>;
  inFileNames_p->push_back(inFileName);
  const Int_t nFiles = (Int_t)inFileNames_p->size();

  Int_t nSysts(0);
  TString expSysts[]={"btagup","btagdn","othertagup","othertagdn","jesup","jesdn","jerup","jerdn","leffup","leffdn"};
  
  //book histograms
  std::map<TString,TH1 *> histos;
  histos["wgtcounter"] = new TH1F("wgtcounter",";Weight;Events;",200,0,200);
  histos["fidcounter"] = new TH1F("fidcounter",";Weight;Events;",200,0,200);
  histos["trig"] = new TH1F("trig",";Trigger;Events",2,0,2);
  histos["lpt"]  = new TH1F("lpt",";Transverse momentum [GeV];Events",20.,0.,200.);
  histos["leta"] = new TH1F("leta",";Pseudo-rapidity;Events",20.,0.,2.1);
  histos["mt"]   = new TH1F("mt",";Transverse Mass [GeV];Events" ,20,0.,200.);
  histos["metpt"]= new TH1F("metpt",";Missing transverse energy [GeV];Events" ,20,0.,200.);

  //electron selection control plots
  for(int ireg=0; ireg<2; ireg++)
    {
      TString pf(ireg==0 ? "_ee" : "_eb");
      histos["sigmaietaieta"+pf]=new TH1F("sigmaietaieta"+pf,";#sigma(i#eta,i#eta);Electrons",25,0,0.04);
      histos["detain"+pf]=new TH1F("detain"+pf,";#Delta#eta(in);Electrons",25,-0.015,0.15);
      histos["dphiin"+pf]=new TH1F("dephiin"+pf,";#Delta#phi(in);Electrons",25,-0.05,0.05);
      histos["hovere"+pf]=new TH1F("hovere"+pf,";H/E;Electrons",25,0,0.05);
      histos["eoverpinv"+pf]=new TH1F("eoverpinv"+pf,";1/E-1/p [GeV^{-1}];Electrons",25,0,0.1);
      histos["d0"+pf]=new TH1F("d0"+pf,";d_{0} [cm];Electrons",25,-0.05,0.05);
      histos["dz"+pf]=new TH1F("dz"+pf,";d_{z} [cm];Electrons",25,-0.5,0.5);
      histos["misshits"+pf]=new TH1F("misshits"+pf,";Missing hits;Electrons",4,0,4);
      histos["convveto"+pf]=new TH1F("convveto"+pf,";Conversion veto flag;Electrons",2,0,2);
      histos["reliso"+pf]=new TH1F("reliso"+pf,";Relative isolation;Electrons",25,0,0.1);
    }

  //per b-tag multiplicity control plots
  for(int ij=0; ij<=2; ij++)
    {
      TString pf(Form("%db",ij));
      histos["lpt_"+pf]    = new TH1F("lpt_"+pf,";Transverse momentum [GeV];Events",5.,20.,120.);
      histos["leta_"+pf]   = new TH1F("leta_"+pf,";Pseudo-rapidity;Events",5.,0.,2.2);
      histos["jpt_"+pf]    = new TH1F("jpt_"+pf,";Transverse momentum [GeV];Events",5.,0.,250.);
      histos["jeta_"+pf]   = new TH1F("jeta_"+pf,";Pseudo-rapidity;Events",5.,0.,2.5);
      histos["ht_"+pf]     = new TH1F("ht_"+pf,";H_{T} [GeV];Events",10.,0.,800.);
      histos["metpt_"+pf]  = new TH1F("metpt_"+pf,";Missing transverse energy [GeV];Events" ,5,0.,200.);
      histos["metphi_"+pf] = new TH1F("metphi_" + pf,";MET #phi [rad];Events" ,5,-3.2,3.2);
      histos["mt_"+pf]     = new TH1F("mt_"+pf,";Transverse Mass [GeV];Events" ,10,0.,300.);
      histos["mjj_"+pf]    = new TH1F("mjj_"+pf,";Mass(j,j') [GeV];Events" ,20,0.,400.);
      histos["mlb_"+pf]    = new TH1F("mlb_"+pf,";Mass(l,b) [GeV];Events" ,20,0.,300.);
      histos["njets_"+pf]  = new TH1F("njets_"+pf,";Jet multiplicity;Events" ,6,2.,8.);

      if(isMC && runSysts)
	{
	  nSysts=sizeof(expSysts)/sizeof(TString);
	  histos["mjjshapes_"+pf+"_exp"]=new TH2F("mjjshapes_"+pf+"_exp",";Mass(j,j');Systematic uncertainty;Events",20,0,400,nSysts,0,nSysts);
	  for(int i=0; i<nSysts; i++)
	    histos["mjjshapes_"+pf+"_exp"]->GetYaxis()->SetBinLabel(i+1,expSysts[i]);
	  
	  histos["mjjshapes_"+pf+"_gen"]=new TH2F("mjjshapes_"+pf+"_gen",";Mass(j,j') [GeV];Systematic uncertainty;Events",20,0,400,200,0,200);
	  for(int i=0; i<200;i++)
	    histos["mjjshapes_"+pf+"_gen"]->GetYaxis()->SetBinLabel(i+1,Form("genUnc%d",i));
	}
    }

  //prepare histograms
  for(std::map<TString,TH1 *>::iterator it=histos.begin();
      it!=histos.end();
      it++)
    {
      it->second->Sumw2();
      it->second->SetDirectory(0);
    }

  for(Int_t fileIter = 0; fileIter < nFiles; fileIter++){

    TString inF(inFileNames_p->at(fileIter));
    if(inF.Contains("/store") && !inF.Contains("root:")) inF="root://eoscms//eos/cms/"+inF;
    TFile* inFile_p = TFile::Open(inF, "READ");
    
    TTree* lepTree_p = (TTree*)inFile_p->Get("ggHiNtuplizer/EventTree");
    TTree* jetTree_p = (TTree*)inFile_p->Get("ak4PFJetAnalyzer/t");
    TTree* hiTree_p = (TTree*)inFile_p->Get("hiEvtAnalyzer/HiTree");
    TTree* hltTree_p = (TTree*)inFile_p->Get("hltanalysis/HltTree");
    TTree *pfCand_p  = (TTree *)inFile_p->Get("pfcandAnalyzer/pfTree");
    
    //PF candidates
    std::vector<int> *pfId_p=0;
    std::vector<float> *pfPt_p=0,*pfEta_p=0,*pfPhi_p=0,*pfEnergy_p=0;
    pfCand_p->SetBranchStatus("pfId",     1);
    pfCand_p->SetBranchStatus("pfPt",     1);
    pfCand_p->SetBranchStatus("pfEta",    1);
    pfCand_p->SetBranchStatus("pfPhi",    1);
    pfCand_p->SetBranchStatus("pfEnergy", 1);
    pfCand_p->SetBranchAddress("pfId",     &pfId_p);
    pfCand_p->SetBranchAddress("pfPt",     &pfPt_p);
    pfCand_p->SetBranchAddress("pfEta",    &pfEta_p);
    pfCand_p->SetBranchAddress("pfPhi",    &pfPhi_p);
    pfCand_p->SetBranchAddress("pfEnergy", &pfEnergy_p);


    //muon variables
    std::vector<float>* muPt_p = 0;
    std::vector<float>* muPhi_p = 0;
    std::vector<float>* muEta_p = 0;
    std::vector<int>* muChg_p = 0;
    std::vector<float>* muChi2NDF_p = 0;
    std::vector<float>* muInnerD0_p = 0;
    std::vector<float>* muInnerDz_p = 0;
    std::vector<int>* muMuonHits_p = 0;
    std::vector<int>* muStations_p = 0;
    std::vector<int>* muTrkLayers_p = 0;
    std::vector<int>* muPixelHits_p = 0;    
    std::vector<float> *muPFChIso_p=0,*muPFPhoIso_p=0,*muPFNeuIso_p=0,*muPFPUIso_p=0;
    lepTree_p->SetBranchStatus("*", 0);
    lepTree_p->SetBranchStatus("muPt", 1);
    lepTree_p->SetBranchStatus("muPhi", 1);
    lepTree_p->SetBranchStatus("muEta", 1);
    lepTree_p->SetBranchStatus("muCharge", 1);
    lepTree_p->SetBranchStatus("muChi2NDF", 1);
    lepTree_p->SetBranchStatus("muInnerD0", 1);
    lepTree_p->SetBranchStatus("muInnerDz", 1);
    lepTree_p->SetBranchStatus("muMuonHits", 1);
    lepTree_p->SetBranchStatus("muStations", 1);
    lepTree_p->SetBranchStatus("muTrkLayers", 1);
    lepTree_p->SetBranchStatus("muPixelHits", 1);    
    lepTree_p->SetBranchStatus("muPFChIso", 1);
    lepTree_p->SetBranchStatus("muPFPhoIso", 1);
    lepTree_p->SetBranchStatus("muPFNeuIso", 1);
    lepTree_p->SetBranchStatus("muPFPUIso", 1);
    lepTree_p->SetBranchAddress("muPt", &muPt_p);
    lepTree_p->SetBranchAddress("muPhi", &muPhi_p);
    lepTree_p->SetBranchAddress("muEta", &muEta_p);
    lepTree_p->SetBranchAddress("muCharge", &muChg_p);
    lepTree_p->SetBranchAddress("muChi2NDF", &muChi2NDF_p);
    lepTree_p->SetBranchAddress("muInnerD0", &muInnerD0_p);
    lepTree_p->SetBranchAddress("muInnerDz", &muInnerDz_p);
    lepTree_p->SetBranchAddress("muMuonHits", &muMuonHits_p);
    lepTree_p->SetBranchAddress("muStations", &muStations_p);
    lepTree_p->SetBranchAddress("muTrkLayers", &muTrkLayers_p);
    lepTree_p->SetBranchAddress("muPixelHits", &muPixelHits_p);    
    lepTree_p->SetBranchAddress("muPFChIso", &muPFChIso_p);
    lepTree_p->SetBranchAddress("muPFPhoIso", &muPFPhoIso_p);
    lepTree_p->SetBranchAddress("muPFNeuIso", &muPFNeuIso_p);
    lepTree_p->SetBranchAddress("muPFPUIso", &muPFPUIso_p);

    //electron variables
    std::vector<float>* elePt_p = 0;
    std::vector<float>* elePhi_p = 0;
    std::vector<float>* eleEta_p = 0;
    std::vector<float>* eleSigmaIEtaIEta_p = 0;
    std::vector<float>* eledEtaAtVtx_p = 0;
    std::vector<float>* eledPhiAtVtx_p = 0;
    std::vector<float>* eleHoverE_p = 0;
    std::vector<float>* eleEoverP_p = 0;
    std::vector<float>* eleD0_p = 0;
    std::vector<float>* eleDz_p = 0;
    std::vector<float>* eleMissHits_p = 0;
    std::vector<float>* elepassConversionVeto_p = 0;
    std::vector<float>* elePFChIso_p=0, *elePFPhoIso_p=0, *elePFNeuIso_p=0, *elePFPUIso_p=0, *eleEffAreaTimesRho_p=0;
    std::vector<int>*   eleIDVeto_p=0;
    std::vector<int>*   eleIDLoose_p=0;
    std::vector<int>*   eleIDMedium_p=0;
    std::vector<int>*   eleIDTight_p=0;
    std::vector<int>*   eleCharge_p=0;
    lepTree_p->SetBranchStatus("elePt", 1);
    lepTree_p->SetBranchStatus("elePhi", 1);
    lepTree_p->SetBranchStatus("eleEta", 1);
    lepTree_p->SetBranchStatus("eleSigmaIEtaIEta", 1);
    lepTree_p->SetBranchStatus("eledEtaAtVtx", 1);
    lepTree_p->SetBranchStatus("eledPhiAtVtx", 1);
    lepTree_p->SetBranchStatus("eleHoverE", 1);
    lepTree_p->SetBranchStatus("eleEoverPInv", 1);
    lepTree_p->SetBranchStatus("eleD0", 1);
    lepTree_p->SetBranchStatus("eleDz", 1);
    lepTree_p->SetBranchStatus("eleMissHits", 1);
    lepTree_p->SetBranchStatus("elepassConversionVeto", 1);
    lepTree_p->SetBranchStatus("elePFChIso", 1);
    lepTree_p->SetBranchStatus("elePFPhoIso", 1);
    lepTree_p->SetBranchStatus("elePFNeuIso", 1);
    lepTree_p->SetBranchStatus("elePFPUIso", 1);
    lepTree_p->SetBranchStatus("eleEffAreaTimesRho", 1);
    lepTree_p->SetBranchStatus("eleID*", 1);
    lepTree_p->SetBranchStatus("eleCharge", 1);
    lepTree_p->SetBranchAddress("elePt", &elePt_p);
    lepTree_p->SetBranchAddress("elePhi", &elePhi_p);
    lepTree_p->SetBranchAddress("eleEta", &eleEta_p);
    lepTree_p->SetBranchAddress("eleSigmaIEtaIEta", &eleSigmaIEtaIEta_p);
    lepTree_p->SetBranchAddress("eledEtaAtVtx", &eledEtaAtVtx_p);
    lepTree_p->SetBranchAddress("eledPhiAtVtx", &eledPhiAtVtx_p);
    lepTree_p->SetBranchAddress("eleHoverE", &eleHoverE_p);
    lepTree_p->SetBranchAddress("eleEoverPInv", &eleEoverP_p);
    lepTree_p->SetBranchAddress("eleD0", &eleD0_p);
    lepTree_p->SetBranchAddress("eleDz", &eleDz_p);
    lepTree_p->SetBranchAddress("eleMissHits", &eleMissHits_p);
    lepTree_p->SetBranchAddress("elepassConversionVeto", &elepassConversionVeto_p);
    lepTree_p->SetBranchAddress("elePFChIso", &elePFChIso_p);
    lepTree_p->SetBranchAddress("elePFPhoIso", &elePFPhoIso_p);
    lepTree_p->SetBranchAddress("elePFNeuIso", &elePFNeuIso_p);
    lepTree_p->SetBranchAddress("elePFPUIso", &elePFPUIso_p);
    lepTree_p->SetBranchAddress("eleEffAreaTimesRho", &eleEffAreaTimesRho_p);
    lepTree_p->SetBranchAddress("eleIDVeto", &eleIDVeto_p);
    lepTree_p->SetBranchAddress("eleIDLoose", &eleIDLoose_p);
    lepTree_p->SetBranchAddress("eleIDMedium", &eleIDMedium_p);
    lepTree_p->SetBranchAddress("eleIDTight", &eleIDTight_p);
    lepTree_p->SetBranchAddress("eleCharge", &eleCharge_p);

    //gen-level variables
    std::vector<int> *mcPID=0;
    std::vector<float> *mcPt=0,*mcEta=0,*mcPhi=0;
    lepTree_p->SetBranchStatus("mcPID", 1);
    lepTree_p->SetBranchStatus("mcPt", 1);
    lepTree_p->SetBranchStatus("mcEta", 1);
    lepTree_p->SetBranchStatus("mcPhi", 1);
    lepTree_p->SetBranchAddress("mcPID", &mcPID);
    lepTree_p->SetBranchAddress("mcPt", &mcPt);
    lepTree_p->SetBranchAddress("mcEta", &mcEta);
    lepTree_p->SetBranchAddress("mcPhi", &mcPhi);

    //jet variables
    const int maxJets = 5000;
    Int_t   nref,ngen;
    Float_t jtpt[maxJets],genpt[maxJets];
    Float_t jteta[maxJets],geneta[maxJets];
    Float_t jtphi[maxJets],genphi[maxJets];
    Float_t jtm[maxJets]; 
    Float_t discr_csvV2[maxJets];
    Float_t refpt[maxJets];
    Int_t refparton_flavor[maxJets];
    jetTree_p->SetBranchStatus("*", 0);
    jetTree_p->SetBranchStatus("ngen", 1);
    jetTree_p->SetBranchStatus("genpt", 1);
    jetTree_p->SetBranchStatus("genphi", 1);
    jetTree_p->SetBranchStatus("geneta", 1);
    jetTree_p->SetBranchStatus("nref", 1);
    jetTree_p->SetBranchStatus("jtpt", 1);
    jetTree_p->SetBranchStatus("jtphi", 1);
    jetTree_p->SetBranchStatus("jteta", 1);
    jetTree_p->SetBranchStatus("jtm", 1);
    jetTree_p->SetBranchStatus("discr_csvV2", 1);
    jetTree_p->SetBranchStatus("refpt", 1);
    jetTree_p->SetBranchStatus("refparton_flavorForB", 1);
    jetTree_p->SetBranchAddress("nref", &nref);
    jetTree_p->SetBranchAddress("jtpt", jtpt);
    jetTree_p->SetBranchAddress("jtphi", jtphi);
    jetTree_p->SetBranchAddress("jteta", jteta);
    jetTree_p->SetBranchAddress("jtm", jtm);
    jetTree_p->SetBranchAddress("discr_csvV2", discr_csvV2);
    jetTree_p->SetBranchAddress("refpt", refpt);
    jetTree_p->SetBranchAddress("refparton_flavorForB", refparton_flavor);
    jetTree_p->SetBranchAddress("ngen", &ngen);
    jetTree_p->SetBranchAddress("genpt", genpt);
    jetTree_p->SetBranchAddress("genphi", genphi);
    jetTree_p->SetBranchAddress("geneta", geneta);

    //event variables
    UInt_t run_, lumi_;
    ULong64_t evt_;
    Int_t hiBin_;
    Float_t vz_;
    Float_t weight;
    std::vector<float> *ttbar_w_p=0;
    hiTree_p->SetBranchStatus("*", 0);
    hiTree_p->SetBranchStatus("run", 1);
    hiTree_p->SetBranchStatus("evt", 1);
    hiTree_p->SetBranchStatus("lumi", 1);
    hiTree_p->SetBranchStatus("hiBin", 1);
    hiTree_p->SetBranchStatus("vz", 1);
    hiTree_p->SetBranchStatus("weight", 1);
    hiTree_p->SetBranchStatus("ttbar_w",1);
    hiTree_p->SetBranchAddress("run", &run_);
    hiTree_p->SetBranchAddress("evt", &evt_);
    hiTree_p->SetBranchAddress("lumi", &lumi_);
    hiTree_p->SetBranchAddress("hiBin", &hiBin_);
    hiTree_p->SetBranchAddress("vz", &vz_);
    hiTree_p->SetBranchAddress("weight", &weight);
    hiTree_p->SetBranchAddress("ttbar_w",&ttbar_w_p);
  
    //trigger
    int trig = 0;
    std::string triggerName;
    if(channelSelection==13 || channelSelection==1300)
      {
	triggerName = isMC ? "HLT_HIL2Mu15ForPPRef_v1" : "HLT_HIL2Mu15_v1"; 
      }
    
    if(channelSelection==11 || channelSelection==1100) 
      {
	triggerName = isMC ? "HLT_HISinglePhoton40_Eta3p1ForPPRef_v1" : "HLT_HISinglePhoton40_Eta3p1_v1";
      }
    
    hltTree_p->SetBranchStatus(triggerName.data(),1);
    hltTree_p->SetBranchAddress(triggerName.data(),&trig);
    
    Int_t nEntries = (Int_t)lepTree_p->GetEntries();
    
    std::cout << "Analysing " << nEntries << " events isMC=" << isMC
	      << " trigger=" << triggerName << endl;

    for(Int_t entry = 0; entry < nEntries; entry++)
      {
	if(entry%1000==0)
	  {
	    printf("\r [%d/%d] done",entry,nEntries);
	    cout << flush;
	  }

	//readout this event
	lepTree_p->GetEntry(entry);
	jetTree_p->GetEntry(entry);
	hiTree_p->GetEntry(entry);
	hltTree_p->GetEntry(entry);
	pfCand_p->GetEntry(entry);

	//assign an event weight
	float evWeight(1.0);
	if(isMC)
	  {
	    if(ttbar_w_p->size()) evWeight = ttbar_w_p->at(0);
	    evWeight *= totalEvtNorm;

	    //fiducial region analysis
	    std::vector<TLorentzVector> selGenLeptons;
	    for(size_t imc=0; imc<mcPID->size(); imc++)
	      {
		int abspid=abs(mcPID->at(imc));
		if(abspid!=13) continue;
		TLorentzVector p4;
		p4.SetPtEtaPhiM(mcPt->at(imc),mcEta->at(imc),mcPhi->at(imc),0.);
		if(p4.Pt()<25 || fabs(p4.Eta())>2.1) continue;
		selGenLeptons.push_back(p4);
	      }
	    
	    //select gen jets cross-cleaning with leading muon
	    int nGenJets(0);
	    for(int imcj=0; imcj<ngen; imcj++)
	      {
		TLorentzVector p4;
		p4.SetPtEtaPhiM(genpt[imcj],geneta[imcj],genphi[imcj],0.);
		if(selGenLeptons.size() && p4.DeltaR(selGenLeptons[0])<0.4) continue;
		if(p4.Pt()<25 || fabs(p4.Eta())>2.4) continue;
		nGenJets++;
	      }

	    //check if it passes the gen level acceptance
	    bool passFid(nGenJets>=2 && selGenLeptons.size()>0);
	    for(size_t iw=0; iw<ttbar_w_p->size(); iw++)
	      {
		histos["wgtcounter"]->Fill(iw,ttbar_w_p->at(iw));
		if(passFid) histos["fidcounter"]->Fill(iw,ttbar_w_p->at(iw));
	      }
	  }

	//require trigger for the event
	histos["trig"]->Fill(trig,evWeight);
	if(trig==0) continue;


	//select good muons
	//cf. details in https://twiki.cern.ch/twiki/bin/view/CMS/SWGuideMuonIdRun2
	std::vector<TLorentzVector> tightMuons,looseMuons,tightMuonsNonIso;
	std::vector<int> muonCharge;
	const Int_t nMu = (Int_t)muPt_p->size();
	for(Int_t muIter = 0; muIter < nMu; muIter++)
	  {
	    bool passLooseKin( muPt_p->at(muIter) > 15. && TMath::Abs(muEta_p->at(muIter))<2.4);
	    bool passTightKin( muPt_p->at(muIter) > 25. && TMath::Abs(muEta_p->at(muIter))<2.1);
	    bool passLooseId(true);
	    bool passTightId( passLooseId
			      && muChi2NDF_p->at(muIter) < 10
			      && muMuonHits_p->at(muIter) >0 		
			      && muStations_p->at(muIter) >1
			      && TMath::Abs(muInnerD0_p->at(muIter))<0.2
			      && TMath::Abs(muInnerDz_p->at(muIter))<0.5
			      && muPixelHits_p->at(muIter)>0		
			    && muTrkLayers_p->at(muIter)>5);
	    float relIso=(muPFChIso_p->at(muIter)+TMath::Max(muPFPhoIso_p->at(muIter)+muPFNeuIso_p->at(muIter)-0.5*muPFPUIso_p->at(muIter),0.))/muPt_p->at(muIter);
	    bool passTightIso( relIso<0.15);
	    bool passLooseIso( relIso<0.25);
	    
	    //save muon if good
	    TLorentzVector p4(0,0,0,0);
	    p4.SetPtEtaPhiM(muPt_p->at(muIter),muEta_p->at(muIter),muPhi_p->at(muIter), 0.1056583715);
	    if(passTightKin && passTightId && !passTightIso && relIso>0.2) 
	      {
		tightMuonsNonIso.push_back(p4);
		muonCharge.push_back(muChg_p->at(muIter));
	      }
	    if(passTightKin && passTightId && passTightIso)     
	      {
		tightMuons.push_back( p4 );
		muonCharge.push_back(muChg_p->at(muIter));
	      }
	    else if(passLooseKin && passLooseId && passLooseIso) looseMuons.push_back( p4 );
	  }
  

	//select good electronss
	//cf. details in https://twiki.cern.ch/twiki/bin/view/CMS/CutBasedElectronIdentificationRun2
	std::vector<TLorentzVector> mediumElectrons,vetoElectrons,mediumElectronsNonIso;
	std::vector<int> elCharge;
	const Int_t nEl = (Int_t)elePt_p->size();
	for(Int_t elIter = 0; elIter < nEl; elIter++)
	  {
	    bool passMediumPt(elePt_p->at(elIter) > 40.0);  
	    bool passMediumEta(fabs(eleEta_p->at(elIter)) < 2.1 && (fabs(eleEta_p->at(elIter)) < 1.4442 || fabs(eleEta_p->at(elIter)) > 1.5660));
	    bool passPt(elePt_p->at(elIter) > 20.0);  
	    bool passEta(fabs(eleEta_p->at(elIter)) < 2.5);
	    if(!passPt || !passEta) continue;
	    bool passMediumId ((fabs(eleEta_p->at(elIter)) <= 1.4479
				&& fabs(eleSigmaIEtaIEta_p->at(elIter)) < 0.0101
				&& fabs(eledEtaAtVtx_p->at(elIter)) < 0.0103
				&& fabs(eledPhiAtVtx_p->at(elIter)) < 0.0336
				&& fabs(eleHoverE_p->at(elIter)) < 0.0876
				&& fabs(eleEoverP_p->at(elIter)) < 0.0174
				&& fabs(eleD0_p->at(elIter)) < 0.0118
				&& fabs(eleDz_p->at(elIter)) < 0.373
				&& fabs(eleMissHits_p->at(elIter)) <= 2 
				&& elepassConversionVeto_p->at(elIter)==1)
			       ||
			       (fabs(eleEta_p->at(elIter)) > 1.4479
				&& fabs(eleSigmaIEtaIEta_p->at(elIter)) < 0.0283
				&& fabs(eledEtaAtVtx_p->at(elIter)) < 0.00733
				&& fabs(eledPhiAtVtx_p->at(elIter)) < 0.114
				&& fabs(eleHoverE_p->at(elIter)) < 0.0678
				&& fabs(eleEoverP_p->at(elIter)) < 0.0898
				&& fabs(eleD0_p->at(elIter)) < 0.0739
				&& fabs(eleDz_p->at(elIter)) < 0.602
				&& fabs(eleMissHits_p->at(elIter)) <= 1 
				&& elepassConversionVeto_p->at(elIter)==1)
			       );
	    bool passVetoId ((fabs(eleEta_p->at(elIter)) <= 1.4479
			      && fabs(eleSigmaIEtaIEta_p->at(elIter)) < 0.0114
			      && fabs(eledEtaAtVtx_p->at(elIter)) < 0.0152
			      && fabs(eledPhiAtVtx_p->at(elIter)) < 0.216
			      && fabs(eleHoverE_p->at(elIter)) < 0.181
			      && fabs(eleEoverP_p->at(elIter)) < 0.207
			      && fabs(eleD0_p->at(elIter)) < 0.0564
			      && fabs(eleDz_p->at(elIter)) < 0.472
			      && fabs(eleMissHits_p->at(elIter)) <= 2 
			      && fabs(elepassConversionVeto_p->at(elIter)))
			     ||
			     (fabs(eleEta_p->at(elIter)) > 1.4479
			      && fabs(eleSigmaIEtaIEta_p->at(elIter)) < 0.0352
			      && fabs(eledEtaAtVtx_p->at(elIter)) < 0.0113
			      && fabs(eledPhiAtVtx_p->at(elIter)) < 0.237
			      && fabs(eleHoverE_p->at(elIter)) < 0.116
			      && fabs(eleEoverP_p->at(elIter)) < 0.174
			      && fabs(eleD0_p->at(elIter)) < 0.222
			      && fabs(eleDz_p->at(elIter)) < 0.921
			      && fabs(eleMissHits_p->at(elIter)) <= 3 
			      && fabs(elepassConversionVeto_p->at(elIter)))
			     );
	    
	    double deposit, corrEA_isolation;
	    deposit =  fabs(elePFPhoIso_p->at(elIter)+elePFNeuIso_p->at(elIter)-eleEffAreaTimesRho_p->at(elIter));
	    corrEA_isolation = (elePFChIso_p->at(elIter) + TMath::Max (0.0, deposit )) / elePt_p->at(elIter);

	    bool passMediumIso( (corrEA_isolation < 0.0766 && fabs(eleEta_p->at(elIter)) <= 1.4479) || (corrEA_isolation < 0.0678 && fabs(eleEta_p->at(elIter)) > 1.4479) );
	    bool passVetoIso( (corrEA_isolation < 0.126 && fabs(eleEta_p->at(elIter)) <= 1.4479) || (corrEA_isolation < 0.144 && fabs(eleEta_p->at(elIter)) > 1.4479) );

	    TString pf(fabs(eleEta_p->at(elIter)) > 1.4479 ? "_ee" : "_eb");
	    if(passMediumId)
	      {
		histos["reliso"+pf]->Fill( corrEA_isolation,evWeight);
	      }
	    if(passMediumIso)
	      {
		histos["sigmaietaieta"+pf]->Fill(eleSigmaIEtaIEta_p->at(elIter),evWeight);
		histos["detain"+pf]->Fill(eledEtaAtVtx_p->at(elIter),evWeight);
		histos["dphiin"+pf]->Fill(eledPhiAtVtx_p->at(elIter),evWeight);
		histos["hovere"+pf]->Fill(eleHoverE_p->at(elIter),evWeight);
		histos["eoverpinv"+pf]->Fill(eleEoverP_p->at(elIter),evWeight);
		histos["d0"+pf]->Fill(eleD0_p->at(elIter),evWeight);
		histos["dz"+pf]->Fill(eleDz_p->at(elIter),evWeight);
		histos["misshits"+pf]->Fill(eleMissHits_p->at(elIter),evWeight);
		histos["convveto"+pf]->Fill(elepassConversionVeto_p->at(elIter),evWeight);
	      }
	    
	
	    //save electron if good
	    TLorentzVector p4(0,0,0,0);
	    p4.SetPtEtaPhiM(elePt_p->at(elIter),eleEta_p->at(elIter),elePhi_p->at(elIter), 0.0510);
	        
	    if (passMediumPt && passMediumEta && passMediumId && corrEA_isolation > 0.2)
	      {
		mediumElectronsNonIso.push_back( p4 );
		elCharge.push_back(eleCharge_p->at(elIter));
	      }
	    else if(passMediumPt && passMediumEta && passMediumId && passMediumIso)
	      {
		mediumElectrons.push_back( p4 );
		elCharge.push_back(eleCharge_p->at(elIter));
	      }
	    else if (passVetoId && passVetoIso)
	      {
		vetoElectrons.push_back( p4 );
		elCharge.push_back(eleCharge_p->at(elIter));
	      }
	  }
	


	//CHANNEL SELECTION
	//muons
	if(channelSelection==1300)
	  {
	    if(tightMuonsNonIso.size()==0) continue;
	    if(tightMuons.size()+looseMuons.size()+mediumElectrons.size()+vetoElectrons.size()!=0) continue;
	    tightMuons=tightMuonsNonIso;
	  }
	if(channelSelection==13)
	  {
	    if(tightMuons.size()!=1) continue; //=1 tight muon	
	    if(looseMuons.size()+mediumElectrons.size()+vetoElectrons.size()!=0) continue; //no extra leptons
	    if(chargeSelection!=0)
	      {
		if(muonCharge[0]!=chargeSelection) continue;
	      }
	  }
	//electrons
	if(channelSelection==1100)
	  {
	    if(mediumElectronsNonIso.size()==0) continue;
	    if(mediumElectrons.size()+vetoElectrons.size()+tightMuons.size()+looseMuons.size()!=0) continue;
	    mediumElectrons=mediumElectronsNonIso;
	  }
	if(channelSelection==11)
	  {
	    if(mediumElectrons.size()!=1) continue; //=1 medium electron
	    if(vetoElectrons.size()+tightMuons.size()+looseMuons.size()!=0) continue; //no extra leptons
	    if(chargeSelection!=0)
	      {
		if(elCharge[0]!=chargeSelection) continue;
	      }
	  }
	
	// combined leptons
	std::vector<TLorentzVector> goodLeptons;
	goodLeptons = (channelSelection==1300 || channelSelection==13) ?  tightMuons : mediumElectrons;  
	if(goodLeptons.size()==0) continue;

	//raw MET (noHF)
	TLorentzVector rawMET(0,0,0,0);	
	for(size_t ipf=0; ipf<pfId_p->size(); ipf++)
	  {
	    Float_t abseta=TMath::Abs(pfEta_p->at(ipf));
	    if(abseta>3.0) continue;
	    rawMET += TLorentzVector(-pfPt_p->at(ipf)*TMath::Cos(pfPhi_p->at(ipf)),
				     -pfPt_p->at(ipf)*TMath::Sin(pfPhi_p->at(ipf)),
				     0,
				     0);
	  }

	//transverse mass
	float mt(computeMT(goodLeptons[0],rawMET));

	histos["lpt"]->Fill(goodLeptons[0].Pt(),evWeight);
	histos["leta"]->Fill(fabs(goodLeptons[0].Eta()),evWeight);
	histos["mt"]->Fill(mt,evWeight);
	histos["metpt"]->Fill(rawMET.Pt(),evWeight);


	//jet counting
	typedef std::vector<TLorentzVector> JetColl_t;
	std::vector<JetColl_t> bJets(9),lightJets(9);
	for(Int_t jetIter = 0; jetIter < nref; jetIter++)
	  {
	    //cross clean with trigger muon
	    TLorentzVector jp4(0,0,0,0);
	    jp4.SetPtEtaPhiM(jtpt[jetIter],jteta[jetIter],jtphi[jetIter],jtm[jetIter]);
	    if(jp4.DeltaR(goodLeptons[0])<0.4) continue;
	    
	    //in tracker region
	    if(TMath::Abs(jp4.Eta())>2.4) continue;

	    //systematic variations
	    Int_t jflav(abs(refparton_flavor[jetIter]));	    
	    bool passCSVM(discr_csvV2[jetIter]>0.8),passCSVMUp(passCSVM),passCSVMDn(passCSVM);	    
	    std::vector<float> jerSmear(3,1.0),jesScaleUnc(3,1.0);
	    if(isMC)
	      {
		//jet energy resolution smearing	
		if(refpt[jetIter]>0) jerSmear=getJetResolutionScales(jp4.Pt(),jp4.Eta(),refpt[jetIter]);
		TLorentzVector rawjp4(jp4);
		jp4 *= jerSmear[0];

		jesScaleUnc[1]=1.028;
		jesScaleUnc[2]=0.972;

		//b-tagging
		float jptforBtag(jp4.Pt()>1000. ? 999. : jp4.Pt());
		if(jflav==5)
		  {
		    float expEff    = expBtagEff["b"]->Eval(jptforBtag); 
		    myBTagSFUtil.modifyBTagsWithSF(passCSVMUp,1.1,expEff);	
		    myBTagSFUtil.modifyBTagsWithSF(passCSVMDn,0.9,expEff);	
		  }
		else if(jflav==4)
		  {
		    float expEff    = expBtagEff["c"]->Eval(jptforBtag); 
		    myBTagSFUtil.modifyBTagsWithSF(passCSVMUp,1.1,expEff);	
		    myBTagSFUtil.modifyBTagsWithSF(passCSVMDn,0.9,expEff);	
		  }
		else
		  {
		    float expEff    = expBtagEff["udsg"]->Eval(jptforBtag); 
		    myBTagSFUtil.modifyBTagsWithSF(passCSVMUp,1.3,expEff);	
		    myBTagSFUtil.modifyBTagsWithSF(passCSVMDn,0.7,expEff);	
		  }		
	      }

	    if(jp4.Pt()>30)
	      {
		//nominal selection
		if(passCSVM) bJets[0].push_back(jp4);
		else         lightJets[0].push_back(jp4);

		//tag variations affect differently depending on the flavour
		if(jflav==5 || jflav==4)
		  {
		    if(passCSVMUp)
		      {
			bJets[1].push_back(jp4);
		      }
		    else
		      {
			lightJets[1].push_back(jp4);
		      }
		    if(passCSVMDn) 
		      {
			bJets[2].push_back(jp4);
		      }
		    else
		      {
			lightJets[2].push_back(jp4);
		      }
		    if(passCSVM)   
		      {
			bJets[3].push_back(jp4);
			bJets[4].push_back(jp4);
		      }
		    else      
		      {
			lightJets[3].push_back(jp4);
			lightJets[4].push_back(jp4);
		      }
		  }
		else
		  {
		    if(passCSVM)   
		      {
			bJets[1].push_back(jp4);
			bJets[2].push_back(jp4);
		      }
		    else      
		      {
			lightJets[1].push_back(jp4);
			lightJets[2].push_back(jp4);
		      }
		    if(passCSVMUp) 
		      {
			bJets[3].push_back(jp4);
		      }
		    else
		      {
			lightJets[3].push_back(jp4);
		      }
		    if(passCSVMDn) 
		      {
			bJets[4].push_back(jp4);
		      }
		    else
		      {
			lightJets[4].push_back(jp4);
		      }
		  }
	      }
	    
	    for(size_t ivar=0; ivar<2; ivar++)
	      {
		//JES varied selections
		TLorentzVector jesVarP4(jp4); jesVarP4*=jesScaleUnc[ivar+1];
		if(jesVarP4.Pt()>30)
		  {
		    if(passCSVM) bJets[5+ivar].push_back(jesVarP4);
		    else         lightJets[5+ivar].push_back(jesVarP4);
		  }

		//JER varied selections
		TLorentzVector jerVarP4(jp4); jerVarP4*=jerSmear[ivar+1]/jerSmear[0];     
		if(jerVarP4.Pt()>30)
		  {
		    if(passCSVM) bJets[7+ivar].push_back(jerVarP4);
		    else         lightJets[7+ivar].push_back(jerVarP4);
		  }
	      }
	  }
	

	//
	for(Int_t ivar=0; ivar<=nSysts; ivar++)
	  {
	    Int_t jetIdx(0);
	    if(ivar>=1 && ivar<=8) jetIdx=ivar;

	    //require at least two light jet acompanying the lepton
	    Int_t nljets(lightJets[jetIdx].size());
	    Int_t nbtags(bJets[jetIdx].size());
	    Int_t njets(nljets+nbtags);
	    
	    if(nljets<2) continue;
	    TString pf(Form("%db",TMath::Min(nbtags,2)));
	    
	    //jet-related quantities
	    Float_t mjj( (lightJets[jetIdx][0]+lightJets[jetIdx][1]).M() );
	    Float_t htsum(0);
	    for(Int_t ij=0; ij<nbtags; ij++) htsum += bJets[jetIdx][ij].Pt();
	    for(Int_t ij=0; ij<nljets; ij++) htsum += lightJets[jetIdx][ij].Pt();

	    Float_t mlb( (goodLeptons[0]+lightJets[jetIdx][0]).M() );
	    if(nbtags>0)
	      {
		mlb=(goodLeptons[0]+bJets[jetIdx][0]).M();
		if(nbtags>1)
		  {
		    mlb=TMath::Min( mlb, Float_t((goodLeptons[0]+bJets[jetIdx][1]).M()) );
		  }
	      }

	    //update event weight if needed
	    Float_t iweight(evWeight);
	    if(ivar==9)  iweight*=1.03;
	    if(ivar==10) iweight*=1.03;
	  
	    //fill histos
	    if(ivar==0)
	      {
		histos["lpt_"+pf]->Fill(goodLeptons[0].Pt(),iweight);
		histos["leta_"+pf]->Fill(fabs(goodLeptons[0].Eta()),iweight);
		histos["ht_"+pf]->Fill(htsum,iweight);
		histos["mjj_"+pf]->Fill(mjj,iweight);
		histos["mlb_"+pf]->Fill(mlb,iweight);
	       
		if(runSysts)
		  {
		    //theory uncertainties (by matrix-element weighting)
		    for(size_t igs=0; igs<ttbar_w_p->size(); igs++)
		      {
			float newWeight( iweight );
			if(isTTJets && normH && normH->GetBinContent(igs+1))
			  {
			    newWeight *= (ttbar_w_p->at(igs)/ttbar_w_p->at(0)) * ( normH->GetBinContent(1)/normH->GetBinContent(igs+1));
			  }
			else
			  {
			    newWeight *= (ttbar_w_p->at(igs)/ttbar_w_p->at(0));
			  }
			((TH2 *)histos["mjjshapes_"+pf+"_gen"])->Fill(mjj,igs,newWeight);
		      }
		  }
		histos["metpt_"+pf]->Fill(rawMET.Pt(),iweight);
		histos["metphi_"+pf]->Fill(rawMET.Phi(),iweight);
		histos["mt_"+pf]->Fill(mt,iweight);    
		if(nbtags)
		  {
		    histos["jpt_"+pf]->Fill(bJets[jetIdx][0].Pt(),iweight);
		    histos["jeta_"+pf]->Fill(fabs(bJets[jetIdx][0].Eta()),iweight);
		  }
		else
		  {
		    histos["jpt_"+pf]->Fill(lightJets[jetIdx][0].Pt(),iweight);
		    histos["jeta_"+pf]->Fill(fabs(lightJets[jetIdx][0].Eta()),iweight);
		  }
		histos["njets_"+pf]->Fill(njets,iweight);
	      }
	    else if (runSysts)
	      {
		((TH2 *)histos["mjjshapes_"+pf+"_exp"])->Fill(mjj,ivar-1,iweight);
	      }
	  }
      }
    
    inFile_p->Close();
    delete inFile_p;    
  }
  
  //dump histograms
  TFile* outFile_p = new TFile(outFileName, "RECREATE");
  outFile_p->cd();
  for(std::map<TString, TH1 *>::iterator it=histos.begin();
      it!=histos.end();
      it++)
    {
      it->second->SetDirectory(outFile_p);
      it->second->Write();
    }
  outFile_p->Close();
  delete outFile_p;
  
  return;
}
