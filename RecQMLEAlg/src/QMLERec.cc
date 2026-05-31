#include "RecQMLEAlg/QMLERec.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include "RecQMLEAlg/QMLEFCN.h"
#include "Math/Minimizer.h"
#include "Math/GSLMinimizer.h"
#include "Math/Functor.h"
#include "Math/Factory.h"
#include "Minuit2/FCNBase.h"
#include "TVector3.h"
#include "TMath.h"
#include "TFile.h"
#include "TF1.h"
#include "TGraph2D.h"
#include "TCanvas.h"
#include <cmath>
#include <numeric>

#include "Event/SimHeader.h"
#include "Event/CdElecHeader.h"
#include "Event/CdElecEvt.h"
#include "Event/CdElecChannel.h"
#include "Event/CdVertexRecEvt.h"
#include "Event/CdVertexRecHeader.h"

#include "Event/CdCalibHeader.h"
#include "Event/CdCalibEvt.h"
#include "Event/CdCalibChannel.h"


#include "SniperKernel/AlgFactory.h"
#include "SniperKernel/SniperPtr.h"
#include "SniperKernel/SniperLog.h"
#include "RootWriter/RootWriter.h"
#include "BufferMemMgr/IDataMemMgr.h"
#include "EvtNavigator/NavBuffer.h"
#include "TAOGeometry/SimGeomSvc.h"

#include <TGeoManager.h>
// #include "RecQMLEAlg/ChargeCenterUtils.hh"

#include <boost/python.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>

DECLARE_ALGORITHM(QMLERec);

namespace {
constexpr double kLegacyPi = 3.1415926;
}

QMLERec::QMLERec(const std::string& name)
    : AlgBase(name),evt(0),m_calibsvc(nullptr)
{
    CD_radius = 900;
    // mGausFunc = new TF1("mGaus", "gaus");

    declProp("ChargeTemplateFile",charge_template_file = "charge_template");
    declProp("CCFactor",cc_factor = 0.66);
    declProp("QSPE",QSPE_factor = 3100);
    declProp("nonuniformity",nonuniformity_file = "uniformityMap.root");   
    declProp("useTrueVertex", useTrueVertex);
    declProp("trueVertexFile", trueVertexFile);
    declProp("rec_Ge_only", rec_Ge_only);
    declProp("Ge_evt_list_file", Ge_evt_list_file);
    // declProp("particle_ID", particle_ID);
    declProp("saveUserFile", m_saveUserFile = false);

}

QMLERec::~QMLERec()
{
    // delete mGausFunc;
}

bool QMLERec::initialize()
{   
    SniperPtr<ICalibSvc> Calib_svc(getParent(), "CalibSvc");
    if (Calib_svc.invalid()) {
       LogError << "can't find service CalibSvc" << std::endl;
       return false;
    }
    m_calibsvc = Calib_svc.data();  
    // std::cout << "enable Charge Info: " << enableChargeInfo << std::endl;
    // std::cout << "Charge center alg. factor : "<<cc_factor<<std::endl;
    // std::cout << "****************e- Template File: e-_" << std::string(charge_template_file) <<std::endl;
    // charge_template = new ChargeTemplate("e-_" + std::string(charge_template_file));
    std::cout << "****************Ge68 Template File: Ge68_" << std::string(charge_template_file) <<std::endl;
    charge_template_ge68 = new ChargeTemplate("Ge68_" + std::string(charge_template_file));
    std::cout << "**************** useTrueVertex: " << std::to_string(useTrueVertex) << ", File Path: " <<  trueVertexFile << std::endl;
    // std::cout << "**************** particle_PDG_ID: " <<  particle_ID << std::endl;

    if(useTrueVertex){
        vertexFile = TFile::Open(trueVertexFile.c_str());
        vertexTree = (TTree*)vertexFile->Get("Event/Sim/SimEvt");
        vertexTree->SetBranchAddress("SimEvt", &SimEvt);
    }

 
// =================================================
    std::string root_dir = getenv("RECQMLEALGROOT");
    // std::string uniformity_filename = root_dir + "/input/" + nonuniformity_file;
    // std::cout << "*************** Uniformity Map: " << uniformity_filename << std::endl;
    // nonuniformityFile = new TFile(uniformity_filename.c_str(), "READ");
    // nonuniformity = (TH2F*)nonuniformityFile->Get("uniformity");
    // =================================================
    // std::string ChannelFilePath((root_dir + "/script/channel_position_v2.txt").c_str());
    // std::cout << "*************** channel file " << std::string(ChannelFilePath) << std::endl;
   //===================================================================

   //dcr list
    // TFile* calibpar_file= TFile::Open((root_dir + "/script/TAO_SiPM_calib_par_1771022040.root").c_str(), "READ");
    // if (!calibpar_file || calibpar_file->IsZombie()) 
    // {
    //     std::cerr << "Failed to open calib file\n";
    //     return false;
    // }
    // TTree* tree=(TTree*)calibpar_file->Get("myevt");
    // tree->SetBranchAddress("dcr", dcr_list.data());
    // tree->SetBranchAddress("rPDE", rPDE_list.data());
    // tree->GetEntry(0);
    // calibpar_file->Close();
    // for (int i = 0; i < CHANNELNUM; i++)
    // {
    //     dcr_list[i] = dcr_list[i] * channel_readout_window; // convert to probability in readout window
    // }


   //bad channel list
//    std::ifstream badchannel_file(root_dir +"/script/ALLBad_channels_20260208_616_T25.7.2_st.txt");
//     int bad_channel;
//     while(badchannel_file >> bad_channel)
//     {
//         bad_channel_list.insert(bad_channel);
//     }
//     badchannel_file.close();

    //Ge event list
    // std::ifstream GeEvt_file(root_dir +"/script/Ge_slec_run1309.txt");
    if (rec_Ge_only)
    {
        std::ifstream GeEvt_file(Ge_evt_list_file.c_str());
        int Ge_event;
        while(GeEvt_file >> Ge_event)
        {
            Ge_event_list.insert(Ge_event);
        }
        GeEvt_file.close();
    }

    // = access the geometry =
    SniperPtr<SimGeomSvc> simgeom_svc(getParent(), "SimGeomSvc");
    // == check exist or not ==
    if (simgeom_svc.invalid()) {
        LogError << "can't find SimGeomSvc" << std::endl;
        return false;
    }
    // == get Sipm & channel parameter
    // // std::vector<TVector3> channelPositionsVec= QMLERec::ReadChannelPositions(ChannelFilePath);
    // std::cout << "*************** channel file " << channelPositionsVec.size() << std::endl;

    m_cdGeom = simgeom_svc->getCdGeom();
    if (!m_cdGeom) {
        LogError << "Failed to get CdGeom from SimGeomSvc" << std::endl;
        return false;
    }
    for(int i = 0;i < 8048; i++)
    {
        SiPMGeom* sipmGeom = m_cdGeom->FindSiPM(i);
        if (!sipmGeom) {
            LogError << "Failed to find SiPM geometry for channel " << i << std::endl;
            // channel_vec.clear();
            return false;
        }
        TVector3 n_pos = sipmGeom->getTileCenter();
        channel_vec[i] = TVector3(n_pos.X() + 2446., 
                                    n_pos.Y() + 2446., 
                                    n_pos.Z() + 8212.8);


         //get SiPM's Phi, unit: rad
        // TVector3 n_pos =channelPositionsVec[i];
        // channel_vec[i] = n_pos;    
           
    }
    LogInfo << "Total SiPM & Channel: " << SIPMNUM << '\t' <<CHANNELNUM<<std::endl;

    if(m_saveUserFile) {
      SniperPtr<RootWriter> rootwriter(getParent(), "RootWriter");
      if (rootwriter.invalid()) {
          LogError << "Can't Find RootWriter. "
                   << std::endl;
          return false;
      }

////////////////////////////////////////////
// fChannelID.clear();
// fChannelHitTime.clear();
// fChannelExpPEVec.clear();
// fChannelHitPEVec.clear();
////////////////////////////////////////////

#ifndef SNIPER_VERSION_2
      evt = rootwriter->bookTree("RECEVT/QMLERec", "user defined data");
#else
      evt = rootwriter->bookTree(*getParent(), "RECEVT/QMLERec", "user defined data");
#endif
      // evt->Branch("evtID", &evtID, "evtID/I");
      evt->Branch("fChannelHit", fChannelHit,"fChannelHit[8048]/D");
      // evt->Branch("fChannelHitPEVec", fChannelHitPEVec, "fChannelHitPEVec[8048]/D");
      evt->Branch("fChannelTotHit", &fChannelTotHit, "fChannelTotHit/D");
      evt->Branch("fChannelExpPE", fChannelExpPE, "fChannelExpPE[8048]/D");   
      // evt->Branch("prob_list", prob_list, "prob_list[8048]/D");   
//      evt->Branch("fChannelExpPEVec", &fChannelExpPEVec);
      // evt->Branch("fChannelID", &fChannelID);                               
      // evt->Branch("fChannelHitTime", &fChannelHitTime); 
      evt->Branch("fExpPE", &fExpPE, "fExpPE/D"); // total expected PE
      evt->Branch("fRecEvis", &fRecEvis, "fRecEvis/D");   //  reconstructed energy by Charge Template method
      evt->Branch("fRecX", &fRecX, "fRecX/D");
      evt->Branch("fRecY", &fRecY, "fRecY/D");
      evt->Branch("fRecZ", &fRecZ, "fRecZ/D");
      // evt->Branch("fRecR", &fRecR, "fRecR/D");
      // evt->Branch("fRecTheta", &fRecTheta, "fRecTheta/D");
      // evt->Branch("fRecPhi", &fRecPhi, "fRecPhi/D");
      evt->Branch("TimeStamp",&TimeStamp,"TimeStamp/D");
      evt->Branch("fCCRecEvis", &fCCRecEvis, "fCCRecEvis/D");   //  reconstructed energy by Charge Center method
      evt->Branch("totalPE", &totalPE, "totalPE/D");   //  reconstructed energy by Charge Center method
      evt->Branch("fCCRecX", &fCCRecX, "fCCRecX/D");
      evt->Branch("fCCRecY", &fCCRecY, "fCCRecY/D");
      evt->Branch("fCCRecZ", &fCCRecZ, "fCCRecZ/D");
      // evt->Branch("fCCRecR", &fCCRecR, "fCCRecR/D");
      // evt->Branch("fCCRecR2", &fCCRecR2, "fCCRecR2/D");
      // evt->Branch("fCCRecTheta", &fCCRecTheta, "fCCRecTheta/D");
      // evt->Branch("fCCRecPhi", &fCCRecPhi, "fCCRecPhi/D");
      evt->Branch("fChi2", &fChi2, "fChi2/D");
      evt->Branch("fEdm", &fEdm, "fEdm/D");
   } 

    // create minimizer
    vtxllfcn = new VertexRecLikelihoodFCN(this);
    vtxllminimizer_migrad = ROOT::Math::Factory::CreateMinimizer("Minuit2","Simplex");
    vtxllminimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2","Simplex");
    return true;
}

bool QMLERec::execute()
{
    // const int maxEvtNum = 1000;
    // if(evtID >= maxEvtNum){
    //     std::cout << "Reached maxEvtNum: " << maxEvtNum << ", stop processing further events." << std::endl;
    //     return false;
    // }
    
    SniperDataPtr<JM::NavBuffer> navBuf(getRoot(), "/Event");
    if (navBuf.invalid()) {
        std::cerr<< "Failed to get NavBuffer for /Event" << std::endl;
        return true;
    }
    LogDebug << "navBuf: " << navBuf.data() << std::endl;

    JM::EvtNavigator* evt_nav = navBuf->curEvt();
    LogDebug << "evt_nav: " << evt_nav << std::endl;
    if (not evt_nav) {
        std::cerr << "Failed to get current event from NavBuffer" << std::endl;
        return true;
    }

    Tao::CdVertexRecHeader* cd_rec_evt_header = NULL;
    Tao::CdVertexRecEvt* cd_rec_evt = NULL;
    // if(! cd_rec_evt_header) {
    //     cd_rec_evt_header = new Tao::CdVertexRecHeader();
    //     cd_rec_evt = new Tao::CdVertexRecEvt();
    // }

    //read esd calib evt
    Tao::CdCalibHeader* cd_calib_hdr = dynamic_cast<Tao::CdCalibHeader*>(evt_nav->getHeader("/Event/Calib"));
    if (not cd_calib_hdr) {
        // std::cerr << "Failed to get CdCalibHeader from NavBuffer" << std::endl;
        return true;
    }
    if (!cd_calib_hdr->hasEvent()) {
        std::cout<<"no data is found, skip this event."<<std::endl;
        return true;
    }
    //read esd rec evt
    Tao::CdVertexRecHeader* esd_ccrec_hdr = dynamic_cast<Tao::CdVertexRecHeader*>(evt_nav->getHeader("/Event/Rec/CCRec"));
    if (not esd_ccrec_hdr) {
        // std::cerr << "Failed to get CdVertexRecHeader from NavBuffer" << std::endl;
        return true;
    }
    if (!esd_ccrec_hdr->hasCdVertexEvent()) {
        std::cout<<"no data is found, skip this event."<<std::endl;
        return true;
    }

    cd_rec_evt_header = new Tao::CdVertexRecHeader();
    cd_rec_evt = new Tao::CdVertexRecEvt();
        
        // == get event ==
        //vertexTree->GetEntry(evtID);
        //Tao::SimTrack *mPrimaryTracks1 = SimEvt->getTracksVec()[0];
        //particle_ID = mPrimaryTracks1->getPDGID();
        // if(particle_ID==-11){Y0=4425.0;}
        // if(particle_ID==11){Y0=4786.0;}
        // Y0=4500;
        Tao::CdCalibEvt* calib_event = dynamic_cast<Tao::CdCalibEvt*>(cd_calib_hdr->event());
        Tao::CdVertexRecEvt* CCRec_event = dynamic_cast<Tao::CdVertexRecEvt*>(esd_ccrec_hdr->cdVertexEvent());
        TimeStamp=evt_nav->TimeStamp();
        evtID += 1;
        // std::cout << "is_GeEvt(" << evtID << ") = " << is_GeEvt(evtID) << std::endl;

        if (CCRec_event->isCompressed())
        {
            // std::cout << "Event " << evtID << " is compressed, skip this event." << std::endl;
            cd_rec_evt -> setPESum(CCRec_event->peSum());   
            cd_rec_evt -> setEnergy(CCRec_event->energy());
            cd_rec_evt -> setEprec(0);
            cd_rec_evt -> setX(CCRec_event->x());
            cd_rec_evt -> setY(CCRec_event->y());
            cd_rec_evt -> setZ(CCRec_event->z());
            cd_rec_evt -> setPx(0);
            cd_rec_evt -> setPy(0);
            cd_rec_evt -> setPz(0);
            cd_rec_evt -> setTimeStamp(TimeStamp);
            cd_rec_evt -> setChisq(0);
            cd_rec_evt -> setIsCompressed(true);
            cd_rec_evt_header -> setCdVertexEvent(cd_rec_evt);
            evt_nav -> addHeader("/Event/Rec/QMLE", cd_rec_evt_header);
            update();
            return true;
        }

        if (rec_Ge_only && !is_GeEvt(evtID))
        {
            std::cout << "Event " << evtID << " is not in Ge event list, skip this event." << std::endl;
            update();
            return true;
        }

        //get bad channel list from CalibSvc
        m_bad_channels = m_calibsvc->GetCDBadChanList();
        for (int i = 0; i < m_bad_channels.size(); i++)
        {
             int bad_channel_id = m_bad_channels[i];
             if (bad_channel_id >= 0 && bad_channel_id < CHANNELNUM) {
                 bad_channel_list[bad_channel_id] = true;
             }
        }

        //read DCR and RelPDE from CalibSvc
        for (int i = 0; i < 8048; ++i) {
            if (m_calibsvc->GetDCR(i) <= 0)
            {
                bad_channel_list[i] = true;
                continue;
            }
            dcr_list[i] = channel_readout_window * m_calibsvc->GetDCR(i);
        }
        for (int i = 0; i < 8048; ++i) {
            if (m_calibsvc->GetRelPDE(i) <= 0)
            {
                bad_channel_list[i] = true;
                continue;
            }
            rPDE_list[i] = m_calibsvc->GetRelPDE(i);
        }

        for (int i=0;i<8048;i++)
        {
            if (bad_channel_list[i]) N_bad_channel += 1;
        }

        std::vector<Tao::CdCalibChannel> channels = calib_event -> GetCalibChannels();
        for (int i = 0; i < channels.size(); i++)
        {
            int id = channels.at(i).CalibgetChannelID();
            // 检查通道ID是否在有效范围内
            if (id < 0 || id >= CHANNELNUM) {
                std::cerr << "Warning: Invalid channel ID " << id << " found, skipping." << std::endl;
                continue;
            }
            if (bad_channel_list[id]) 
            {
                continue;
            }
            std::vector<float> PEs = channels.at(i).CalibgetPEs(); // Get Charge from single channel, unit: p.e.
            double tot_PEs = 0;
            for (int j = 0; j < PEs.size(); j++)
            {
                tot_PEs += pe_count(PEs.at(j));
            }

            fChannelHit[id] = tot_PEs;
            fChannelTotHit += tot_PEs;
            hit_ch_totdcr += dcr_list[id];
            // fChannelID.push_back(id);
            // fChannelHitTime.push_back(channels.at(i).CalibgetTDCs()[0]);
            // fChannelHitPEVec.push_back(tot_PEs);
        }

      
    if(useTrueVertex){
        vertexTree->GetEntry(evtID);
    }
    
    std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Event[" << evtID << "] vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << std::endl;

    TStopwatch T_fit;

    // T_cc.Start();
    fCCRecEvis = CCRec_event->energy();
    fCCRecX = CCRec_event->x();
    fCCRecY = CCRec_event->y();
    fCCRecZ = CCRec_event->z();
    TVector3 cc_vec(fCCRecX, fCCRecY, fCCRecZ);
    fCCRecR = cc_vec.Mag();
    fCCRecTheta = (cc_vec.Theta()) * 180 / TMath::Pi();
    fCCRecPhi = (atan2(fCCRecY,fCCRecX)) * 180 / TMath::Pi();
    // std::cout << "************* Charge Center: "  << "CCEvis = " << fCCRecEvis <<",\tCC vertex(R, Theta, Phi) = "<< "("<<fCCRecR<<", "<< fCCRecTheta <<", "<< fCCRecPhi <<")"<<std::endl;

    // CalChargeCenter();
    // T_cc.Stop();
   
    // std::cout << "************* Visible energy: "<< fChannelTotHit/Y0 <<std::endl;
    // start reconstruction
    T_fit.Start();
    VertexMinimize();
    T_fit.Stop();

    std::cout << "Vertex Fit Time: " << T_fit.RealTime() << " s" << std::endl;
    // fill the event.
    if(evt) evt->Fill();

    // EDM output
    cd_rec_evt -> setPESum(fChannelTotHit);   
    cd_rec_evt -> setEnergy(fRecEvis);
    cd_rec_evt -> setEprec(0);
    cd_rec_evt -> setX(fRecX);
    cd_rec_evt -> setY(fRecY);
    cd_rec_evt -> setZ(fRecZ);
    cd_rec_evt -> setPx(0);
    cd_rec_evt -> setPy(0);
    cd_rec_evt -> setPz(0);
    cd_rec_evt -> setTimeStamp(TimeStamp);
    cd_rec_evt -> setChisq(fChi2);
    cd_rec_evt -> setIsCompressed(false);

    ////////
    // cd_rec_evt -> setChannelId(fChannelID);
    // cd_rec_evt -> setChannelHitTime(fChannelHitTime);
    // cd_rec_evt -> setChannelExpPE(fChannelExpPEVec);
    // cd_rec_evt -> setChannelHitPE(fChannelHitPEVec);
    ////////
    // cd_rec_evt -> setEnergyQuality(0);
    // cd_rec_evt -> setPositionQuality(0); 
    cd_rec_evt_header -> setCdVertexEvent(cd_rec_evt);
    // evt_nav -> addHeader("/Event/Rec/ChargeTemplate", cd_rec_evt_header);
    evt_nav -> addHeader("/Event/Rec/QMLE", cd_rec_evt_header);

    // update here
    update();
    return true;
}

bool QMLERec::finalize()
{
    if(useTrueVertex){
    vertexFile->Close();
    }
    charge_template_ge68->finalize();
    
    return true;
}

double QMLERec::QMLE(
    double Evis,double vr,double vtheta,double vphi)    // vtheta=(0,PI)
{
    QMLEInput input;
    input.charge_template_ge68 = charge_template_ge68;
    input.GeEvis = 0.9250;
    input.PDE = PDE;
    input.ESF = ESF;
    input.saturation = saturation;
    input.channels.resize(CHANNELNUM);

    int active_channels = 0;
    for (int i = 0; i < CHANNELNUM; ++i) {
        input.fChannelHit[i] = fChannelHit[i];
        input.channels[i].posX = channel_vec[i].X();
        input.channels[i].posY = channel_vec[i].Y();
        input.channels[i].posZ = channel_vec[i].Z();
        input.channels[i].rPDE = rPDE_list[i];
        input.channels[i].dcr = dcr_list[i];
        input.channels[i].bad = bad_channel_list[i];
        if (!bad_channel_list[i]) {
            ++active_channels;
        }
    }

    m_Likelihood = ComputeQMLE(input, Evis, vr, vtheta, vphi);

    const char* fixture_path = std::getenv("QMLE_CAPTURE_FCN_FIXTURE");
    static bool fixture_captured = false;
    const bool capture_fixture = fixture_path && !fixture_captured
        && ((active_channels > 0 && fChannelTotHit > 0.) || std::getenv("QMLE_CAPTURE_FCN_FIXTURE_DEBUG"));

    if (const char* trace_path = std::getenv("QMLE_CAPTURE_FCN_TRACE")) {
        static int trace_call = 0;
        FILE* tf = std::fopen(trace_path, "a");
        if (tf) {
            std::fprintf(tf, "call=%d active_channels=%d fChannelTotHit=%.17g FCN=%.17g Evis=%.17g vr=%.17g vtheta=%.17g vphi=%.17g capture=%d\n",
                         ++trace_call, active_channels, fChannelTotHit, m_Likelihood, Evis, std::fabs(vr), vtheta, vphi,
                         static_cast<int>(capture_fixture));
            std::fclose(tf);
        }
    }

    if (capture_fixture) {
        FILE* f = std::fopen(fixture_path, "w");
        if (f) {
            std::fprintf(f, "Evis=%.17g\nvr=%.17g\nvtheta=%.17g\nvphi=%.17g\n", Evis, std::fabs(vr), vtheta, vphi);
            std::fprintf(f, "GeEvis=%.17g\nPDE=%.17g\nESF=%.17g\nsaturation=%.17g\n", input.GeEvis, input.PDE, input.ESF, input.saturation);
            std::fprintf(f, "active_channels=%d\nfChannelTotHit=%.17g\n", active_channels, fChannelTotHit);
            for (int i = 0; i < CHANNELNUM; ++i) {
                std::fprintf(f, "%d %.17g %d %.17g %.17g %.17g %.17g %.17g\n",
                             i, input.fChannelHit[i], static_cast<int>(input.channels[i].bad),
                             input.channels[i].rPDE, input.channels[i].dcr,
                             input.channels[i].posX, input.channels[i].posY, input.channels[i].posZ);
            }
            std::fprintf(f, "END_CH\nFCN=%.17g\n", m_Likelihood);
            std::fclose(f);
            fixture_captured = true;
        }
    }
    return m_Likelihood;
}




bool QMLERec::update()
{
    m_Likelihood = 0.;
    fChannelTotHit = 0.;
    hit_ch_totdcr = 0.;
    N_bad_channel = 0;
    for(int i=0; i < CHANNELNUM; i++) {
        // prob_list[i] = 0.;
        fChannelHit[i] = 0.;
        fChannelExpPE[i] = 0.;
        bad_channel_list[i] = false;
        dcr_list[i] = 0.;
        rPDE_list[i] = 0.;
    }
    // fChannelID.clear();
    // fChannelHitTime.clear();
    // fChannelExpPEVec.clear();
    // fChannelHitPEVec.clear();
    return true;
}

bool QMLERec::VertexMinimize()
{

    ROOT::Math::Functor vtxllf(*vtxllfcn,4);

    vtxllminimizer_migrad->SetFunction(vtxllf);
    vtxllminimizer_migrad->SetMaxFunctionCalls(1e5);
    vtxllminimizer_migrad->SetMaxIterations(1e5);
    vtxllminimizer_migrad->SetTolerance(1.e-5);
    vtxllminimizer_migrad->SetStrategy(2);
    vtxllminimizer_migrad->SetPrintLevel(0);
    vtxllminimizer_migrad->SetPrecision(1e-8);
    // Calculate initialize value
    double fCCRadius = fCCRecR;
    if (fCCRadius > 900)
    {
        fCCRadius = 890;
    } 
    
    double exp_hit_init = fChannelTotHit;
    // exp_hit_init -= CHANNELNUM * dark_noise_prob;
    exp_hit_init -= hit_ch_totdcr;
    int goodness = 0;
    // use migrad to minimize
    // vtxllminimizer_migrad->SetLimitedVariable(0, "Evis", fChannelTotHit/Y0, 0.01, 0, 15);
    vtxllminimizer_migrad->SetLimitedVariable(0, "Evis", exp_hit_init/Y0, 0.01, 0, 200);
    // vtxllminimizer_migrad->SetFixedVariable(0, "Evis", exp_hit_init/Y0);//固定能量
    if(!useTrueVertex){
        vtxllminimizer_migrad->SetLimitedVariable(1,"radius",fCCRadius, 0.5,-899,899);
        // vtxllminimizer_migrad->SetVariable(1,"radius",fCCRadius, 0.5);
        //vtxllminimizer_migrad->SetFixedVariable(1,"radius",fCCRadius);
        // vtxllminimizer_migrad->SetLimitedVariable(2,"theta",fCCRecTheta*TMath::Pi()/180, 0.01, 0, TMath::Pi());
        // vtxllminimizer_migrad->SetLimitedVariable(3,"phi",fCCRecPhi*TMath::Pi()/180, 0.01, 0, 2*TMath::Pi());
        vtxllminimizer_migrad->SetFixedVariable(2,"theta",fCCRecTheta*TMath::Pi()/180);
        vtxllminimizer_migrad->SetFixedVariable(3,"phi",fCCRecPhi*TMath::Pi()/180);
    } else{
        Tao::SimTrack* mPrimaryTracks = SimEvt->getTracksVec()[0];
        double QedepR = sqrt( pow(mPrimaryTracks->getQEdepX(), 2) + pow(mPrimaryTracks->getQEdepY(), 2) + pow(mPrimaryTracks->getQEdepZ(), 2) );
        vtxllminimizer_migrad->SetFixedVariable(1,"radius", QedepR);
        vtxllminimizer_migrad->SetFixedVariable(2,"theta", acos( mPrimaryTracks->getQEdepZ() / QedepR ));
        vtxllminimizer_migrad->SetFixedVariable(3,"phi", atan2( mPrimaryTracks->getQEdepY() , mPrimaryTracks->getQEdepX() ));
    }
    
    goodness = vtxllminimizer_migrad->Minimize();
    // std::cout << "Acc. Vertex Minimize :: Goodness = " << goodness << std::endl;
    std::cout << "Minimizer calls: " << vtxllminimizer_migrad->NCalls() << std::endl;
    const double *xs = vtxllminimizer_migrad->X();

    TVector3 v_rec(0,0,1);
    v_rec.SetMagThetaPhi(xs[1],xs[2],xs[3]);
    fRecEvis = xs[0];
    fRecX    = v_rec.X();
    fRecY    = v_rec.Y();
    fRecZ    = v_rec.Z();
    fRecR    = xs[1];
    fRecTheta    = xs[2]*180/TMath::Pi();
    fRecPhi    = xs[3]*180/TMath::Pi();
        
    // fChi2    = vtxllminimizer_migrad->MinValue(); 
///////////////////////////////////////
    // double exp_dark_noise = dark_noise_prob;
    // calculate some value that is needed.
    TVector3 v_vec = TVector3(0, 0 ,1);
    v_vec.SetMagThetaPhi(fRecR,fRecTheta*TMath::Pi()/180,fRecPhi*TMath::Pi()/180);
    fExpPE = 0.;
    double angle;
    double exp_hit;
    for(int i=0; i < CHANNELNUM; i++)
    {
        if (bad_channel_list[i]) continue;
        angle = v_vec.Angle(channel_vec[i]);  
        //if (angle*180/TMath::Pi()>170)continue;
        exp_hit = CalExpChargeHit(fabs(fRecR), angle * 180.0 / kLegacyPi, fRecTheta, fRecEvis);
        exp_hit = exp_hit * rPDE_list[i];
        exp_hit += dcr_list[i];
        if(exp_hit > saturation){
            exp_hit = saturation;
        }
        fExpPE += exp_hit;
        fChannelExpPE[i] = exp_hit;
        // fChannelExpPEVec.push_back(exp_hit);
        
        // std::cout << "channel["<< i <<"] measured Charge: " << fChannelHit[i] << "\texpected nPE: " << exp_hit << "\tPro_Q: " << Pro_Qi_arr[i] <<std::endl;
    }
    fEdm     = vtxllminimizer_migrad->Edm();
    double ndf=8048-N_bad_channel-4; //自由度=通道数-坏通道数-拟合参数数
    double chi2_poisson = 0.0;

    for(int i = 0; i < CHANNELNUM; i++)
    {
        if (bad_channel_list[i]) continue;

        double k = fChannelHit[i];
        double lambda = fChannelExpPE[i];

        if (lambda <= 0) continue; // 防止除零

        chi2_poisson += LogPoisson(k, lambda);
    }
    fChi2=chi2_poisson / ndf;
    // std::cout << "QMLE Evis = " << fRecEvis << " (R, Theta, Phi) = (" << fRecR << ", " << fRecTheta << ", " << fRecPhi << ")"<<std::endl;
    // std::cout << "Likelihood (Minuit) = " << fChi2 << std::endl;
    // std::cout << "Chi2/ndf (Poisson) = " << fChi2
    //       << "   (Chi2=" << chi2_poisson 
    //       << ", ndf=" << ndf << ")"
    //       << "   Edm=" << fEdm << std::endl;
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // if(fRecEvis<0.883){
    //     energy_Ge = fRecEvis; energy_Ek =0.;
    // }else{
    //     energy_Ge = 0.883; energy_Ek=fRecEvis-0.883;
    // }
    return true;
}

bool QMLERec::CalChargeCenter()
{

}



// bool QMLERec::CalChargeCenter()
// {
//     TVector3 cc_vec(0,0,0);
//     for(int i=0; i < CHANNELNUM; i++){
//         int channel_id = i;
//         cc_vec += fChannelHit[i] * channel_vec[channel_id];
//     }
//     cc_vec *= (1.0/fChannelTotHit);
//     double exp_dark_noise = std::accumulate(dcr_list.begin(), dcr_list.end(), 0.0);
//     double cor_factor = fChannelTotHit/(fChannelTotHit - exp_dark_noise);
//     cc_vec *= cor_factor / cc_factor;
//     fCCRecX = cc_vec.X();
//     fCCRecY = cc_vec.Y();
//     fCCRecZ = cc_vec.Z();
//     fCCRecR = cc_vec.Mag();
//     fCCRecTheta = (cc_vec.Theta()) * 180 / TMath::Pi();
//     fCCRecPhi = (atan2(fCCRecY,fCCRecX)) * 180 / TMath::Pi();
//     double ori_fCCRecR=fCCRecR;

//     fCCRecR = calculateNewFccRecR(ori_fCCRecR, ori_fCCRecR, "multicurve");
//     if (fCCRecR >= 900) {
//         fCCRecR = newFccRecrRandom(fCCRecR);
//     }
    
//     fCCRecR2 = (sqrt(fCCRecR+290.62998)-17.41149)/0.013784;
//     fCCRecX = fCCRecR*sin(fCCRecTheta*TMath::Pi()/180)*cos(fCCRecPhi*TMath::Pi()/180);
//     fCCRecY = fCCRecR*sin(fCCRecTheta*TMath::Pi()/180)*sin(fCCRecPhi*TMath::Pi()/180);
//     fCCRecZ = fCCRecR*cos(fCCRecTheta*TMath::Pi()/180);
//     //////************************************************************************ */
//     double rawEnergy = fChannelTotHit / Y0;      

//     double rmin = nonuniformity->GetXaxis()->GetXmin();
//     double rmax = nonuniformity->GetXaxis()->GetXmax();
//     Double_t nonuniformityValue = 0.0;
//     if (fCCRecR < rmin || fCCRecR > rmax) {
//         nonuniformityValue = 1.0;
//     } 
//     else {
//         nonuniformityValue = nonuniformity->Interpolate(fCCRecR, fCCRecTheta);
//     }
//     // Double_t nonuniformityValue = nonuniformity->Interpolate(fCCRecR, fCCRecTheta);
//     double uniformEnergy = rawEnergy / nonuniformityValue;   // 非均匀性修正
//     double Enrec = uniformEnergy * 1; // 非线性修正

//     fCCRecEvis = Enrec;
//     /////
//     if(useTrueVertex){
//         Tao::SimTrack* mPrimaryTracks = SimEvt->getTracksVec()[0];
//         double QedepR = sqrt( pow(mPrimaryTracks->getQEdepX(), 2) + pow(mPrimaryTracks->getQEdepY(), 2) + pow(mPrimaryTracks->getQEdepZ(), 2) );
//         std::cout << "************* True Vertex(R, Theta, Phi) = (" << QedepR << ", " << acos( mPrimaryTracks->getQEdepZ() / QedepR ) << ", " << atan2( mPrimaryTracks->getQEdepY() , mPrimaryTracks->getQEdepX() ) << ")" <<std::endl;
//     }
//     std::cout << "************* Charge Center: "  << "CCEvis = " << fCCRecEvis <<",\tCC vertex(R, Theta, Phi) = "<< "("<<fCCRecR<<", "<< fCCRecTheta <<", "<< fCCRecPhi <<")"<<std::endl;
    
// } 
 
double QMLERec::LogPoisson(double obj,double exp_n)
{
    // likelihood ratio
    double p=2*(exp_n-obj);
    if(obj>0.01){
        p+=2*obj*TMath::Log(obj/exp_n);
    }
    return p;
}
std::vector<TVector3>  QMLERec::ReadChannelPositions(const TString& channelfilepath) {
    std::vector<TVector3> channelPositionVec;

    //
    std::ifstream channelFile1(channelfilepath.Data());
    if (!channelFile1.is_open()) {
        std::cerr << "Error: Failed to open file: " << channelfilepath << std::endl;
        return channelPositionVec;
    }
    int index = 0;
    double x = 0.0, y = 0.0, z = 0.0, R = 0.0, theta = 0.0, phi = 0.0;
    const double offsetX = -2446.0;
    const double offsetY = -2446.0;
    const double offsetZ = -8212.8;    
    while (channelFile1 >> index >> x >> y >> z >> R >> theta >> phi) {
        TVector3 channelPos;
        channelPos.SetXYZ(x - offsetX, y - offsetY, z - offsetZ);
        channelPositionVec.push_back(channelPos);
    }
    channelFile1.close();

    return channelPositionVec;
} 

double QMLERec::CalExpChargeHit(double radius, double theta_sipm, double vtheta, double Evis) // theta_sipm, vtheta unit: 角度制
{

    double exp_hit_ge68 = charge_template_ge68->CalExpChargeHit(radius, theta_sipm, vtheta);
    // double exp_hit_e =charge_template-> CalExpChargeHit(radius, theta_sipm, vtheta);

    double GeEvis ; // mean value of Qedep from detSim
    // if(particle_ID==-11)
    // {
    //   GeEvis=0.9250;//0.6200;
    //   m_average_PE = (Evis * exp_hit_ge68)/GeEvis;
    // }
    // if(particle_ID==11)
    // {
    //    GeEvis=0.99;//0.6612;
    //    m_average_PE = (Evis * exp_hit_e)/GeEvis;
    // }
    GeEvis=0.9250;//0.6200;
    m_average_PE = (Evis * exp_hit_ge68)/GeEvis;

    m_expected_hit = m_average_PE * PDE / ESF;

    return m_expected_hit;
}
