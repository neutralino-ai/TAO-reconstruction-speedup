#include "RecQMLEAlg/ChargeTemplate.h"

#include "TFile.h"
#include "TH3F.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>

ChargeTemplate::ChargeTemplate(std::string charge_tmp_file)
    : charge_template_file(std::move(charge_tmp_file))
{
    initialize();
}

ChargeTemplate::~ChargeTemplate()
{
    finalize();
}

bool ChargeTemplate::initialize()
{
    if (tmp3d != nullptr) {
        return true;
    }

    const char* root_dir = std::getenv("RECQMLEALGROOT");
    if (root_dir == nullptr) {
        return false;
    }

    const std::string file_name = std::string(root_dir) + "/input/" + charge_template_file + ".root";
    tmp_file = TFile::Open(file_name.c_str(), "READ");
    if (tmp_file == nullptr || tmp_file->IsZombie()) {
        return false;
    }

    tmp3d = dynamic_cast<TH3F*>(tmp_file->Get("h3_interpolation"));
    return tmp3d != nullptr;
}

bool ChargeTemplate::finalize()
{
    if (tmp_file != nullptr) {
        tmp_file->Close();
        delete tmp_file;
        tmp_file = nullptr;
        tmp3d = nullptr;
    }
    return true;
}

float ChargeTemplate::CalExpChargeHit(float radius, float theta_sipm, float vtheta)
{
    constexpr float eps = 0.1F;
    theta_sipm = std::clamp(theta_sipm, 0.0F + eps, 180.0F - eps);
    radius = std::clamp(radius, 0.0F + eps, 899.99F - eps);
    vtheta = std::clamp(vtheta, 0.0F + eps, 180.0F - eps);

    return static_cast<float>(tmp3d->Interpolate(theta_sipm, radius, vtheta));
}
