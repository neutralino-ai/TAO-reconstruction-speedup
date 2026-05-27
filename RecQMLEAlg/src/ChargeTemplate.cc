#include "RecQMLEAlg/ChargeTemplate.h"
#include "TFile.h"
#include "math.h"
#include "TString.h"
#include "TVector3.h"
#include <iostream>
#include <fstream>
#include <string>
#include "RecQMLEAlg/Functions.h"

using namespace std;

ChargeTemplate::ChargeTemplate(string charge_tmp_file)
{
    tmp_num = TEMPLATENUM;
    cd_radius = 900;
    sipm_radius = 939.5;
    max_tmp_radius = 0;
    tmp_numbers = 0;
    charge_template_file = charge_tmp_file;
    initialize();
}

ChargeTemplate::~ChargeTemplate()
{
    
}

bool ChargeTemplate::initialize()
{
    string root_dir = getenv("RECQMLEALGROOT");
    // ifstream info_file((root_dir + "/input/" + charge_template_file + ".txt").c_str());
    string file_name = "/input/" + charge_template_file +".root";
    tmp_file = new TFile((root_dir + file_name).c_str());
    std::cout << "****************Template File::::" << std::string(file_name) <<std::endl;

    // double radius = 0;
    // char tmp_name[30];
    // while(info_file >> radius >> tmp_name)
    // { 
    //     tmp_radius.push_back(radius);
    //     TH1F* hist = (TH1F*) tmp_file->Get(tmp_name);
    //     hist->GetBinContent(1);
    //     tmp.push_back(hist);
    //     // cout << "Template radius : "<< radius <<" name : "<< tmp_name <<endl;
    //     tmp_numbers ++;
    //     max_tmp_radius = radius;
    // }
    // // cout << "Max template radius : " << max_tmp_radius << "\nTemplate numbers : "<< tmp_numbers << endl;
    // info_file.close();

    //read 3D template
    tmp3d = (TH3F*) tmp_file->Get("h3_interpolation");
    if (tmp3d == nullptr) {
        std::cerr << "Error: Failed to read 3D template from file: " << file_name << std::endl;
        return false;
    }

    cout << "ChargeTemplate Initialization Finished !!!" << endl;
    return true;
}

bool ChargeTemplate::finalize()
{
    tmp_file->Close();
    return true;
}

TH1F* ChargeTemplate::get_template(int index)
{
    return tmp[index];
}

float ChargeTemplate::get_template_radius(int index)
{
    return tmp_radius[index];
}

float ChargeTemplate::cal_sipm_proj(float radius, float sipm_distance)
{
    float cos_theta_proj = (sipm_distance*sipm_distance + sipm_radius*sipm_radius - radius*radius)/(2*sipm_distance*sipm_radius);
    return cos_theta_proj;
}

float ChargeTemplate::cal_sipm_distance(float radius, float theta)
{
    float cos_theta = cos(theta*PI/180);
    float d = sqrt(sipm_radius*sipm_radius + radius*radius - 2*radius*sipm_radius*cos_theta);
    return d;
}

float ChargeTemplate::LinearInterpolation(float radius, float x0, float y0, float x1, float y1)
{
    float value = 0;
    if (fabs(x0 - x1) < 1.e-2)
    {
        value = (y0 + y1)/2.0;
    }else{
        value = y0 + (radius - x0)*(y1 - y0)/(x1 - x0);
    }
    return value;
}

int ChargeTemplate::FindBeforeIndex(float radius, int low, int high)
{
    if (low == high){
        if(radius < tmp_radius[low]){
            return max(low - 1, 0);
        }else{
            return low;
        }
    }else if(high < low){
        return max(low - 1, 0);
    }
    int mid = int((low + high)/2);
    if(radius >= tmp_radius[mid]){
        return FindBeforeIndex(radius, mid + 1, high);
    }else{
        return FindBeforeIndex(radius, low, mid - 1);
    }

}

// float ChargeTemplate::CalExpChargeHit(float radius, float theta_sipm, float vtheta)
// {

//     static int call_count = 0;
//     call_count++;
//     if (call_count <= 5) {
//         std::cout << "===== CalExpChargeHit CALLED [" << call_count << "] =====" << std::endl;
//         std::cout << "  radius=" << radius << ", theta_sipm=" << theta_sipm << ", vtheta=" << vtheta << std::endl;
//         std::cout << "  tmp3d=" << tmp3d << std::endl;
//     }

//     // 参数说明：
//     // radius: 顶点半径 (mm)，对应TH3F的Y轴
//     // theta_sipm: 顶点与SiPM夹角 (deg)，对应TH3F的X轴
//     // vtheta: 顶点极角 (deg)，对应TH3F的Z轴

//     // 获取TH3F的实际axis范围
//     auto xaxis = tmp3d->GetXaxis();
//     auto yaxis = tmp3d->GetYaxis();
//     auto zaxis = tmp3d->GetZaxis();

//     float h_xmin = xaxis->GetXmin();
//     float h_xmax = xaxis->GetXmax();
//     float h_ymin = yaxis->GetXmin();
//     float h_ymax = yaxis->GetXmax();
//     float h_zmin = zaxis->GetXmin();
//     float h_zmax = zaxis->GetXmax();

//     // 打印TH3F的实际范围（仅第一次）
//     static bool printed = false;
//     if (!printed) {
//         std::cout << "TH3F axis ranges:" << std::endl;
//         std::cout << "  X (theta_sipm): [" << h_xmin << ", " << h_xmax << "]" << std::endl;
//         std::cout << "  Y (radius): [" << h_ymin << ", " << h_ymax << "]" << std::endl;
//         std::cout << "  Z (vtheta): [" << h_zmin << ", " << h_zmax << "]" << std::endl;
//         printed = true;
//     }

//     // // 固定边界值（clamp范围）
//     // const float xmin = 0.0025;
//     // const float xmax = 179.9975;
//     // const float ymin = 0.0;
//     // const float ymax = 899.99;
//     // const float zmin = 0.0;
//     // const float zmax = 180.0;

//     float eps = 1e-6;

//     // clamp到合法范围
//     theta_sipm = std::clamp(theta_sipm, h_xmin + eps, h_xmax - eps);
//     radius = std::clamp(radius, h_ymin + eps, h_ymax - eps);
//     vtheta = std::clamp(vtheta, h_zmin + eps, h_zmax - eps);

//     // 使用FindBin检查bin是否在有效范围内
//     int binx = tmp3d->GetXaxis()->FindBin(theta_sipm);
//     int biny = tmp3d->GetYaxis()->FindBin(radius);
//     int binz = tmp3d->GetZaxis()->FindBin(vtheta);
//     int nbinsx = tmp3d->GetNbinsX();
//     int nbinsy = tmp3d->GetNbinsY();
//     int nbinsz = tmp3d->GetNbinsZ();

//     // 检查bin是否在有效范围内（bin 1到Nbins）
//     bool out_of_range = (binx < 1 || binx > nbinsx ||
//                          biny < 1 || biny > nbinsy ||
//                          binz < 1 || binz > nbinsz);

//     static int error_count = 0;
//     if (out_of_range && error_count < 10) {
//         std::cerr << "Error [" << error_count << "]: Input out of TH3F bin range!" << std::endl;
//         std::cerr << "  theta_sipm=" << theta_sipm << " -> binx=" << binx << " (valid: 1-" << nbinsx << ")" << std::endl;
//         std::cerr << "  radius=" << radius << " -> biny=" << biny << " (valid: 1-" << nbinsy << ")" << std::endl;
//         std::cerr << "  vtheta=" << vtheta << " -> binz=" << binz << " (valid: 1-" << nbinsz << ")" << std::endl;
//         std::cerr << "  Axis ranges: X=[" << h_xmin << "," << h_xmax << "] Y=[" << h_ymin << "," << h_ymax << "] Z=[" << h_zmin << "," << h_zmax << "]" << std::endl;
//         error_count++;
//     }

//     // 3D插值: Interpolate(x, y, z) -> (theta_sipm, radius, vtheta)
//     float temp_hit = tmp3d->Interpolate(theta_sipm, radius, vtheta);

//     return temp_hit;
// }


float ChargeTemplate::CalExpChargeHit(float radius, float theta_sipm, float vtheta)
{
    // 参数说明：
    // radius: 顶点半径 (mm)，对应TH3F的Y轴
    // theta_sipm: 顶点与SiPM夹角 (deg)，对应TH3F的X轴
    // vtheta: 顶点极角 (deg)，对应TH3F的Z轴

    // 固定边界值
    const float xmin = 0.0;
    const float xmax = 180.0;
    const float ymin = 0.0;
    const float ymax = 899.99;
    const float zmin = 0.0;
    const float zmax = 180.0;

    float eps = 0.1;

    // clamp到合法范围
    theta_sipm = std::clamp(theta_sipm, xmin + eps, xmax - eps);
    radius = std::clamp(radius, ymin + eps, ymax - eps);
    vtheta = std::clamp(vtheta, zmin + eps, zmax - eps);

    // 3D插值: Interpolate(x, y, z) -> (theta_sipm, radius, vtheta)
    float temp_hit = tmp3d->Interpolate(theta_sipm, radius, vtheta);

    return temp_hit;
}

// float ChargeTemplate::CalExpChargeHit(float radius, float theta)    // theta --顶点与SiPM之间的夹角
// {

//     int bindex = FindBeforeIndex(radius,0,tmp_numbers-1);   // 获取小于等于0的template的索引值
//     int findex = bindex + 1;
//     if(radius >= max_tmp_radius)
//     {
//         findex = tmp_numbers - 1;
//         bindex = findex - 1;
//     }
//     float cos_theta = cos(theta*PI/180);
//     float sin_theta = sin(theta*PI/180);
//     float sipm_distance = cal_sipm_distance(radius, theta); // 顶点到SiPM之间的距离
//     float correct_factor = cal_sipm_proj(radius, sipm_distance)/pow(sipm_distance,2);   // 修正系数，因为顶点不是垂直入射SiPM，即顶点入射方向与SiPM法线之间存在夹角α。计算顶点投影在SiPM的立体角。当SiPM表面积足够小时，可以近似有立体角Ω=(Acosα)/（R*R）,A是SiPM表面积，R是SiPM到球心的距离，即探测器半径

//     // before charge template information
//     float b_tmp_radius = get_template_radius(bindex);
//     float b_sipm_distance = cal_sipm_distance(b_tmp_radius, theta);
//     float b_correct_factor = cal_sipm_proj(b_tmp_radius, b_sipm_distance)/pow(b_sipm_distance,2);
//     TH1F* b_temp = get_template(bindex);
//     float b_temp_hit = b_temp -> Interpolate(theta) * correct_factor / b_correct_factor;

//     // after charge template information
//     float f_tmp_radius = get_template_radius(findex);
//     float f_sipm_distance = cal_sipm_distance(f_tmp_radius, theta);
//     float f_correct_factor = cal_sipm_proj(f_tmp_radius, f_sipm_distance)/pow(f_sipm_distance,2);
//     TH1F* f_temp = get_template(findex);
//     float f_temp_hit = f_temp -> Interpolate(theta) * correct_factor / f_correct_factor;    
    
//     // get linear interpolation
//     float exp_hit = LinearInterpolation(radius, b_tmp_radius, b_temp_hit, f_tmp_radius, f_temp_hit);
    
//     return exp_hit;
// }
