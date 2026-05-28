#include "RecQMLEAlg/QMLEFCN.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double ComputeQMLE(const QMLEInput& input,
                   double Evis, double vr, double vtheta, double vphi)
{
    vr = std::fabs(vr);

    TVector3 v_vec(0, 0, 1);
    v_vec.SetMagThetaPhi(vr, vtheta, vphi);

    double likelihood = 0.0;

    for (int i = 0; i < 8048; ++i) {
        if (input.channels[i].bad)
            continue;

        const auto& ch = input.channels[i];

        // Angle between vertex direction and channel direction
        double angle = v_vec.Angle(ch.pos);

        // 1. Get raw Ge68 template expectation (no energy scaling yet)
        float exp_hit_ge68 = input.charge_template_ge68->CalExpChargeHit(
            static_cast<float>(vr),
            static_cast<float>(angle * 180.0 / M_PI),
            static_cast<float>(vtheta * 180.0 / M_PI));

        // 2. Energy scaling: same as QMLERec::CalExpChargeHit
        double m_average_PE = (Evis * static_cast<double>(exp_hit_ge68)) / input.GeEvis;
        double exp_hit = m_average_PE * input.PDE / input.ESF;

        // 3. Apply channel-specific rPDE and DCR
        exp_hit = exp_hit * ch.rPDE + ch.dcr;

        if (exp_hit > input.saturation)
            exp_hit = input.saturation;
        if (exp_hit < 1e-10)
            exp_hit = 1e-10;

        // Poisson log-likelihood: P(k|λ) = λ^k e^{-λ} / k!
        // -2 ln P = -2 (k ln λ - λ - ln Γ(k+1))
        double k = input.fChannelHit[i];
        if (k < 0) k = 0;

        double logP = -exp_hit + k * std::log(exp_hit) - std::lgamma(k + 1);
        if (!std::isfinite(logP))
            logP = -1e10;

        double Prob = std::exp(logP);
        if (Prob < 1e-16) Prob = 1e-16;

        likelihood -= 2.0 * std::log(Prob);
    }

    return likelihood;
}