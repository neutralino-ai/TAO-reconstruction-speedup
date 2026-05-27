#ifndef QMLERec_h
#define QMLERec_h

#undef _POSIX_C_SOURCE // to remove warning message of redefinition
#undef _XOPEN_SOURCE // to remove warning message of redefinition

#include "TTree.h"
#include "SniperKernel/AlgBase.h"
#include "SniperKernel/AlgFactory.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include "CalibSvc/ICalibSvc.h"
#include "TAOGeometry/SimGeomSvc.h"
#include "TAOGeometry/CdGeom.h"
#include <TObject.h>
#include "Math/Minimizer.h"
#include "Math/Functor.h"
#include "Math/Factory.h"
#include "Minuit2/FCNBase.h"
#include <vector>
#include <string>
#include "TH2F.h"
#include "TF1.h"
#include "Event/SimEvt.h"
#include "Event/SimTrack.h"
// #include "RecQMLEAlg/ChargeCenterUtils.hh"

#define NSCAN 360
#define SIPMNUM 4024
#define CHANNELNUM SIPMNUM*2

/*
 * QMLERec
 */

class QMLERec : public AlgBase
{
    public:
        QMLERec(const std::string & name);
        ~QMLERec();  
        
        // bool in_badlist(int channelId)
        // {
        //     return bad_channel_list.find(channelId) != bad_channel_list.end();
        // }

        bool is_GeEvt(int id)
        {
            int id_this=id-1;
            return Ge_event_list.find(id_this) != Ge_event_list.end();
        }

        float pe_count(float npe)
        {
            if (npe<0.5) 
            {
                return 0.0;
            }
            return std::round(npe);
        }

        double LogPoisson(double obj,double exp);
        // void CorrectVertex(double& hits, double& r, double& theta, double& phi, double& ge68_alpha);

        bool initialize();
        bool execute();
        bool finalize();

        // update some state
        bool update();
        
        // Charge Center Reconstruction
        bool CalChargeCenter();

        // Use Minimizer for vertex reconstruction
        bool VertexMinimize();
        class VertexRecLikelihoodFCN: public ROOT::Minuit2::FCNBase {
            public:
                VertexRecLikelihoodFCN(QMLERec* rec_alg) { m_alg = rec_alg; }
                double operator() (const std::vector<double>& x) const{
                    return m_alg->QMLE(x[0],x[1],x[2],x[3]);
                }
                double operator() (const double *x) const{
                    std::vector<double> p(x,x+4);
                    return (*this)(p);
                }
                double Up() const { return 0.5; }
            private:
                QMLERec* m_alg;
        };

        // charge expectation
        double CalExpChargeHit(double radius, double theta_sipm, double vtheta, double Evis);

        double QMLE(double Evis,double vr,double vtheta,double vphi);
        std::vector<TVector3> ReadChannelPositions(const TString& filePath1);
        std::string ChannelFilePath;

    private:
        int m_iEvt;
        // input
        ChargeTemplate* charge_template;
        ChargeTemplate* charge_template_ge68;
        double CD_radius;
        TFile *nonuniformityFile = NULL;
        TH2F * nonuniformity = NULL;

        std::string nonuniformity_file;
        bool useTrueVertex;
        
        std::string trueVertexFile;
        std::string chargecenterFile;
        TFile *vertexFile = NULL;
        TTree *vertexTree = NULL;

        Tao::SimEvt* SimEvt = NULL;
        Tao::SimTrack* mPrimaryTracks = NULL;
        
        //minimizer
        ROOT::Math::Minimizer* vtxllminimizer;
        ROOT::Math::Minimizer* vtxllminimizer_migrad;
        VertexRecLikelihoodFCN* vtxllfcn; 
        
        // result
        TTree* evt;
        bool m_saveUserFile;
        int evtID=0;
        double fChannelTotHit = 0.;
        double fChannelHit[8048] = {0.};
        ////
        double fChannelExpPE[8048] = {0.};
        std::vector<Int_t> fChannelID;
        std::vector<double> fChannelHitTime;
        // std::vector<double> fChannelExpPEVec;
        double fChannelHitPEVec[8048]={0.};
        /////
        double fExpPE;   // total expect number of PE
        double fRecEvis;
        double fRecNHit;
        double fRecX;
        double fRecY;
        double fRecZ;
        double fRecR;
        double fRecTheta;    //(0, pi) unit: rad
        double fRecPhi;      //(-pi, pi) unit: rad
        double fCCRecEvis,CCRecEvis;
        double fCCRecX,CCRecX;
        double fCCRecY,CCRecY;
        double fCCRecZ,CCRecZ;
        double fCCRecR,CCRecR;
        double fCCRecR2,CCRecR2;
        double fCCRecTheta,CCRecTheta;
        double fCCRecPhi,CCRecPhi;
        double totalPE,CCtotalPE,diffPerformanceR,CCdiffPerformanceR;
        double diffPerformanceE,CCdiffPerformanceE;
        double QEdep;
        double fChi2;
        double fEdm;
        double TimeStamp;

        // params to control the alg
        std::string charge_template_file;
        double cc_factor;
        double QSPE_factor; 
        

        // center detector geometry
        CdGeom*  m_cdGeom;
        unsigned int SiPMNum = SIPMNUM;   
        int ChannelNum = CHANNELNUM;
        SiPMGeom* m_SiPMGeom;
        // std::vector<TVector3> channelPositionsVec;
        //param of channel
        // double channel_area = 48*24;//72*16 mm^2
        // double channel_noise = 20;                       //Hz/mm^2
        // double channel_readout_window = 1000 * 1.e-9;    //s
        // double channel_rate = 70000;
        double channel_readout_window = 1008 * 1.e-9;    //s
        // double dark_noise_prob = channel_area * channel_noise * channel_readout_window;
        // double dark_noise_prob= channel_rate * channel_readout_window;
        // double dark_noise_prob = 0.;
        double m_average_PE = 0.;
        double m_expected_hit = 0.;
        

        double saturation = 1.e5;
        double SiPMRadius = 939.515;    //mm
        double channel_theta[CHANNELNUM] = {0};
        double channel_phi[CHANNELNUM] = {0};
        TVector3 channel_vec[CHANNELNUM];
        double channel_n_theta[CHANNELNUM] = {0};
        double channel_n_phi[CHANNELNUM] = {0};
        TVector3 channel_n_vec[CHANNELNUM];

        //////
        double ESF =1.;  // energy scale factor
        double PDE =1.;  // 
        int particle_ID;

        // double ES = 3458/1.022; // energy scale, npe of Ge68 is 3458
        double Y0=4500;// = 4852; // photoelectron yield, unit:p.e./MeV 

        //QMLE
        double QThreshold = 0.5; // unit : 0.5 p.e.
        double Q1 = 1.0;    //  mean of SPES, unit: p.e.
        // double S1 = 0.1562;    //  sigma of SPES, unit: p.e.
        double S1 = 0.14;    //  sigma of SPES, unit: p.e.
        double probPrcs = 1.e-10;
        double m_Likelihood = 0.;
        // double energy_Ge = 0.;
        // double energy_Ek = 0.;

        // Data members
        ICalibSvc* m_calibsvc;
        // std::array<float, 8048> m_dcrs;
        // std::array<float, 8048> m_rpdes;
        std::vector<int> m_bad_channels;
        // std::vector<channelPosition> m_channelPosition_vec;

        //dcr list
        double hit_ch_totdcr = 0.;
        std::array<float, CHANNELNUM> dcr_list;
        //rPDE list
        std::array<float, CHANNELNUM> rPDE_list;
        //probability per ch per evt
        double prob_list[CHANNELNUM] = {0.};
        //bad channel list
        int N_bad_channel = 0;
        std::array<bool, 8048> bad_channel_list={false};
        //Ge event list
        std::unordered_set<int> Ge_event_list;

        // TF1* mGausFunc;

        bool rec_Ge_only ;
        std::string Ge_evt_list_file;

};

#endif
