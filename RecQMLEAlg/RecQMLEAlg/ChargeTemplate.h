#ifndef ChargeTemplate_h
#define ChargeTemplate_h

#include <string>

class TFile;
class TH3F;

class ChargeTemplate {
public:
    explicit ChargeTemplate(std::string charge_template_file);
    ~ChargeTemplate();

    bool initialize();
    bool finalize();

    float CalExpChargeHit(float radius, float theta_sipm, float vtheta);

private:
    std::string charge_template_file;
    TFile* tmp_file = nullptr;
    TH3F* tmp3d = nullptr;
};

#endif
