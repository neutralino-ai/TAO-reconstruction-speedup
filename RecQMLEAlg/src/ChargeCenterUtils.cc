#include "RecQMLEAlg/ChargeCenterUtils.hh"

float channel_area = 48*24;                  //mm^2 72*16;   
float channel_noise = 20;                       //Hz/mm^2
float channel_readout_window = 1000 * 1.e-9;    //s
float dark_noise_prob = channel_area * channel_noise * channel_readout_window;
//
TString BasePath = getenv("RECQMLEALGROOT");
TString ChanelFile_path = BasePath + "/script/channel_position_v2.txt";
TString ChanelFile2_path = BasePath + "/script/add_position_v2.txt";

std::unordered_map<int, int> SuPPoint_CHPoint_indexMap = {
    {8048, 0}, {8049, 2}, {8050, 2}, {8051, 4},
    {8052, 6}, {8053, 6}, {8054, 8}, {8055, 10},
    {8056, 10}, {8057, 12}, {8058, 14}, {8059, 14},
    {8060, 16}, {8061, 18}, {8062, 18}, {8063, 20},
    {8064, 22}, {8065, 22}, {8066, 24}, {8067, 26},
    {8068, 26}, {8069, 28}, {8070, 30}, {8071, 30},
    // 下半球
    {8099, 8017}, {8100, 8019}, {8101, 8019}, {8102, 8021},
    {8103, 8023}, {8104, 8023}, {8105, 8025}, {8106, 8027},
    {8107, 8027}, {8108, 8029}, {8109, 8031}, {8110, 8031},
    {8111, 8033}, {8112, 8035}, {8113, 8035}, {8114, 8037},
    {8115, 8039}, {8116, 8039}, {8117, 8041}, {8118, 8043},
    {8119, 8043}, {8120, 8045}, {8121, 8047}, {8122, 8047}
};
// 增补点内的通道的map映射
std::unordered_map<int, int> SuPPoint_SuPPoint_indexMap = {
    // the upper opening's supplemental Channels
    {8072, 8048}, {8073, 8050}, {8074, 8051}, {8075, 8053},
    {8076, 8054}, {8077, 8056}, {8078, 8058}, {8079, 8059},
    {8080, 8061}, {8081, 8062}, {8082, 8064}, {8083, 8066},
    {8084, 8067}, {8085, 8069}, {8086, 8070},
    //
    {8087, 8072}, {8088, 8074}, {8089, 8075}, {8090, 8077},
    {8091, 8079}, {8092, 8080}, {8093, 8082}, {8094, 8084},
    {8095, 8085},
    //
    {8096, 8087}, {8097, 8090}, {8098, 8093},    
    // the lower opening's supplemental Channels
    {8123, 8099}, {8124, 8101}, {8125, 8102}, {8126, 8104},
    {8127, 8105}, {8128, 8107}, {8129, 8109}, {8130, 8110},
    {8131, 8112}, {8132, 8113}, {8133, 8115}, {8134, 8117},
    {8135, 8118}, {8136, 8120}, {8137, 8121},
    //
    {8138, 8123}, {8139, 8125}, {8140, 8126}, {8141, 8128},
    {8142, 8130}, {8143, 8131}, {8144, 8133}, {8145, 8135},
    {8146, 8136},
    //
    {8147, 8138}, {8148, 8141}, {8149, 8144}

};

// calculate the spherical coordinates from the Cartesian coordinates
std::array<double, 3> xyz2rthetaphi(double x, double y, double z) {
    std::array<double, 3> rtp;

    // Calculate r
    rtp[0] = std::sqrt(x*x + y*y + z*z);

    // Calculate theta
    rtp[1] = std::acos(z / rtp[0]) * 180 / M_PI;

    // Calculate rho
    double rho = std::sqrt(x*x + y*y);

    // Calculate phi
    // rtp[2] = std::acos(x / rho) * 180 / M_PI;
    rtp[2]=std::atan2(y,x)*180/M_PI;

    // Adjust phi values
    if (rtp[2]>180 || rtp[2]<-180) {
        cout << "phi is out of range: " << rtp[2] << endl;
        rtp[2] = 360 - rtp[2];
    }

    return rtp;
}
//

// Find the nearest channel index
void FindNearChannelIndices(int index_SuppCH,
                           const std::unordered_map<int, int>& SuPPoint_CHPoint_indexMap,
                           const std::unordered_map<int, int>& SuPPoint_SuPPoint_indexMap,
                           std::vector<int>& IndexVec) {
    if ((index_SuppCH <= 8071 && index_SuppCH >= 8048) || 
        (index_SuppCH <= 8122 && index_SuppCH >= 8099)) {
        auto it = SuPPoint_CHPoint_indexMap.find(index_SuppCH);
        IndexVec.push_back(it->second);
    }
    else if ((index_SuppCH <= 8086 && index_SuppCH >= 8072) || 
        (index_SuppCH <= 8137 && index_SuppCH >= 8123)) {
        auto it = SuPPoint_SuPPoint_indexMap.find(index_SuppCH);
        int index_pre = it->second;
        auto it2 = SuPPoint_CHPoint_indexMap.find(index_pre);
        int index_pre_pre = it2->second;
        IndexVec.push_back(index_pre);
        IndexVec.push_back(index_pre_pre);
        // std::cout << index_pre << ", " << index_pre_pre << std::endl;
    }
    else if ((index_SuppCH <= 8098 && index_SuppCH >= 8087) || 
        (index_SuppCH <= 8149 && index_SuppCH >= 8138)) {
        auto it = SuPPoint_SuPPoint_indexMap.find(index_SuppCH);
        int index_pre = it->second; 
        auto it2 = SuPPoint_SuPPoint_indexMap.find(index_pre);
        int index_pre_pre = it2->second;
        IndexVec.push_back(index_pre);
        IndexVec.push_back(index_pre_pre);
    }
    else {
        std::cerr << "Error: Invalid index_SuppCH: " << index_SuppCH << std::endl;
    }
}

// return the matching indices, if the distance is within 10mm
std::vector<int> PrintDistanceMatchingIndices(const std::vector<EdepCH_DistanceData>& distancesVec, double Distance2) {
    std::vector<int> matchingIndices;
    for (const auto& data : distancesVec) {
        if (std::abs(Distance2 - data.distance) < 10) {
            matchingIndices.push_back(data.index);
        }
    }
    return matchingIndices; 

}
// calculate the distance between EdepPoint and all the channels
void CalculateEdepCHDistance(const TVector3& EdepPoint, std::vector<EdepCH_DistanceData>& distancesVec) {
    std::ifstream ChannelFile(ChanelFile_path.Data());
    if (!ChannelFile.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }
    int index;
    double x, y, z, R, theta, phi;
    while (ChannelFile >> index >> x >> y >> z >> R >> theta >> phi) {
        TVector3 Channel_Pos;
        x=x- (-2446.);
        y=y- (-2446.);
        z=z- (-8212.8);
        Channel_Pos.SetXYZ(x, y, z);
        double EdepP_distance_SipmCH = (Channel_Pos - EdepPoint).Mag();

        distancesVec.emplace_back(index, x, y, z, R, theta, phi, EdepP_distance_SipmCH);
    }
    ChannelFile.close();
}

//
// calculate the PE of the supplemental channels
void CalculateSuppChannelPEs(const TString& filename, const TVector3& QEdepPoint,
                              const std::vector<EdepCH_DistanceData>& distancesVec,
                              double* fChannelHitPE) {
    std::ifstream channelFile2(filename.Data());
    if (!channelFile2.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    int index;
    double x, y, z, R, theta, phi;
    std::vector<int> IndexVec;
    // cout<<"QEdepPoint is: "<<QEdepPoint.x()<<", "<<QEdepPoint.y()<<", "<<QEdepPoint.z()<<endl;
    while (channelFile2 >> index >> x >> y >> z >> R >> theta >> phi) {
        int index_SuppCH = index;
        IndexVec.clear();
        // calculate the position of the supplemental channel
        TVector3 SuppChannel_Pos;
        SuppChannel_Pos.SetXYZ(x - (-2446.), y - (-2446.), z - (-8212.8));
        // calculate the distance between QEdepPoint and the supplemental channel
        double EdepP_distance_SuppCH = (SuppChannel_Pos - QEdepPoint).Mag();
        IndexVec = PrintDistanceMatchingIndices(distancesVec, EdepP_distance_SuppCH);
        
        int IndexVecSize = IndexVec.size();

        ////////
        if (IndexVecSize == 0 ) {
            FindNearChannelIndices(index_SuppCH, SuPPoint_CHPoint_indexMap, SuPPoint_SuPPoint_indexMap, IndexVec);
            IndexVecSize = IndexVec.size();
            if (IndexVecSize == 1) {
                fChannelHitPE[index_SuppCH]=fChannelHitPE[IndexVec[0]];
            }
            else if (IndexVecSize == 2) {
                double hit = 2*fChannelHitPE[IndexVec[0]] - fChannelHitPE[IndexVec[1]];
                if (hit >= 0) fChannelHitPE[index_SuppCH] = hit;
                else fChannelHitPE[index_SuppCH] = fChannelHitPE[IndexVec[0]];
                
            }
            else {
                cout << "Error: Invalid IndexVecSize: " << IndexVecSize << endl;
            }
        }
        else {

            double sumPE_sym = 0;
            // Calculate the sum of PE values of the matching channels
            for (int i = 0; i < IndexVecSize; i++) {
                int index = IndexVec[i];
                sumPE_sym += fChannelHitPE[index];
            }

            // Calculate the average PE value of the matching channels
            if (IndexVecSize > 0) {
                double avgPE_sym = sumPE_sym / IndexVecSize;            
                fChannelHitPE[index_SuppCH] = avgPE_sym;
            } else {
                fChannelHitPE[index_SuppCH] = 0; // No matching channels, set PE to 0
            }

        }
    }

    channelFile2.close();
}
/*== calculate new fCCRecR (Curve Correction) ==*/
double calculateNewFccRecR(double fCCRecR, double ori_fCCRecR, const std::string& Pattern) {
    double new_fccRecR;
    if (Pattern == "multicurve") {
        // Find the corresponding parameters for the given range
        for (const auto& [range, params] : multiCurveParams) {
            if (fCCRecR >= range.first && fCCRecR <= range.second) {
                new_fccRecR = fitFunction(ori_fCCRecR, params);
                break;
            }
        }
    } else if (Pattern == "AllCalibcurve") {
        new_fccRecR = fitFunction(ori_fCCRecR, AllCalibcurveParams);
    } else if (Pattern == "CLScurve") {
        new_fccRecR = fitFunction(ori_fCCRecR, CLScurveParams);
    } else if (Pattern == "ACUcurve") {
        new_fccRecR = fitFunction(ori_fCCRecR, ACUcurveParams);
    } else if (Pattern == "None") {
        new_fccRecR = ori_fCCRecR;
    } else {
        std::cerr << "Error: Invalid pattern \"" << Pattern << "\". Please choose a valid option.\n";
        std::cerr << "Valid options are: \"multicurve\", \"AllCalibcurve\", \"CLScurve\", \"ACUcurve\", \"None\".\n";
        return -1; 
    }

    return new_fccRecR;
}
//

// 2D interpolation using fit function
double FitFuncinterpolate2D(double inputX, double inputY, const std::string& selection) {

    auto it = fitParamsTable.find(selection);
    if (it == fitParamsTable.end()) {
        std::cerr << "Warning: Invalid selection \"" << selection << "\". Using default value 1.\n";
        return 1; 
    }
    const FitParams& params = it->second;
    return interpolateZ(inputX, inputY, params);
}

// 2D interpolation using TGraph2D
double interpolate2D(TGraph2D* graph, double inputX, double inputY) {
    if (!graph) {
        std::cerr << "Error: Invalid TGraph2D object!, please check the input." << std::endl;
        return 1;  
    }
    // Interpolate the Z value
    double interpolatedZ = graph->Interpolate(inputX, inputY);
    if (interpolatedZ <= 0) {
        interpolatedZ = FitFuncinterpolate2D(inputX, inputY, std::string("Cs137_Edep"));
    }
    return interpolatedZ;  
}

double newFccRecrRandom(double new_fccRecR) {
    std::string rootPath = getenv("RECCHARGECENTERALGROOT");
    std::string FileName = rootPath + "/script/openEffsample_beyond900Points.root";
    TFile *func_file = new TFile(FileName.c_str(), "read");
    TH1F *QEdepR_ifFccRbeyond900_merge = (TH1F*)func_file->Get("QEdepR_ifFccRbeyond900");
    new_fccRecR = QEdepR_ifFccRbeyond900_merge->GetRandom();
    return new_fccRecR;
}