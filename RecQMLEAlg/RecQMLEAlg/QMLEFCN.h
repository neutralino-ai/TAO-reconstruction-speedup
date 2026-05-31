#ifndef QMLE_FCN_H
#define QMLE_FCN_H

#include <vector>

class ChargeTemplate;

constexpr int kQMLEChannelCount = 8048;

struct QMLEChannelData {
    double posX = 0.0;
    double posY = 0.0;
    double posZ = 0.0;
    double rPDE = 0.0;
    double dcr = 0.0;
    bool bad = true;
};

struct QMLEInput {
    ChargeTemplate* charge_template_ge68 = nullptr;
    double GeEvis = 0.9250;
    double PDE = 1.0;
    double ESF = 1.0;
    double saturation = 1.e5;
    double fChannelHit[kQMLEChannelCount] = {0.0};
    std::vector<QMLEChannelData> channels;
};

double ComputeQMLE(const QMLEInput& input,
                   double Evis,
                   double vr,
                   double vtheta,
                   double vphi);

#endif  // QMLE_FCN_H
