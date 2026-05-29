#include "RecQMLEAlg/QMLEFCN.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr double LOG_1E_MINUS_16 = -36.841361487904734;  // log(1e-16)

void PrecomputeChannelCache(const QMLEInput& input,
                            double vr, double vtheta,
                            QMLEPerChannelCache& cache)
{
    vr = std::fabs(vr);
    int n = 0;

    double vx = vr * std::sin(vtheta);
    double vz = vr * std::cos(vtheta);

    for (int i = 0; i < 8048; ++i) {
        if (input.channels[i].bad) continue;
        const auto& ch = input.channels[i];

        double dx = vx - ch.posX;
        double dz = vz - ch.posZ;
        double dy = -ch.posY;
        double distCh = std::sqrt(ch.posX*ch.posX + ch.posY*ch.posY + ch.posZ*ch.posZ);
        double dot = vx*ch.posX + vz*ch.posZ;
        double cosAngle = dot / (vr * distCh);
        cosAngle = std::clamp(cosAngle, -1.0, 1.0);
        double angle = std::acos(cosAngle);

        float exp_hit_ge68 = input.charge_template_ge68->CalExpChargeHit(
            static_cast<float>(vr),
            static_cast<float>(angle * 180.0 / M_PI),
            static_cast<float>(vtheta * 180.0 / M_PI));

        double scale = static_cast<double>(exp_hit_ge68) * input.PDE / (input.GeEvis * input.ESF);
        cache.expHitPerUnitEnergy[n] = scale * ch.rPDE;
        cache.activeIndices[n] = i;
        ++n;
    }
    cache.nActive = n;
}

// ---------------------------------------------------------------------------
// Poisson log-likelihood helper (inlined by compiler)
// Returns -2 ln P(k|λ), skipping the exp+log round-trip.
// ---------------------------------------------------------------------------
static inline double poissonNeg2LogL(double k, double lambda) {
    // log P(k|λ) = k·log(λ) - λ - log Γ(k+1)
    // We want -2·logP, clamped at -2·log(1e-16) ≈ 73.68
    if (k == 0.0) {
        // For k=0: log P = -λ, so -2logP = 2λ
        return 2.0 * lambda;
    }
    double logP = -lambda + k * std::log(lambda) - std::lgamma(k + 1.0);
    if (!std::isfinite(logP)) logP = LOG_1E_MINUS_16;
    if (logP < LOG_1E_MINUS_16) logP = LOG_1E_MINUS_16;
    return -2.0 * logP;
}

double ComputeQMLE_Fast(const QMLEInput& input,
                        const QMLEPerChannelCache& cache,
                        double Evis, double vphi)
{
    (void)vphi;
    double likelihood = 0.0;

    for (int j = 0; j < cache.nActive; ++j) {
        int i = cache.activeIndices[j];
        const auto& ch = input.channels[i];
        double exp_hit = Evis * cache.expHitPerUnitEnergy[j] + ch.dcr;
        if (exp_hit > input.saturation) exp_hit = input.saturation;
        if (exp_hit < 1e-10) exp_hit = 1e-10;

        double k = input.fChannelHit[i];
        if (k < 0) k = 0;
        likelihood += poissonNeg2LogL(k, exp_hit);
    }
    return likelihood;
}

double ComputeQMLE(const QMLEInput& input,
                   double Evis, double vr, double vtheta, double vphi)
{
    vr = std::fabs(vr);
    TVector3 v_vec(0, 0, 1);
    v_vec.SetMagThetaPhi(vr, vtheta, vphi);
    double likelihood = 0.0;

    for (int i = 0; i < 8048; ++i) {
        if (input.channels[i].bad) continue;
        const auto& ch = input.channels[i];
        TVector3 tv(ch.posX, ch.posY, ch.posZ);
        double angle = v_vec.Angle(tv);

        float exp_hit_ge68 = input.charge_template_ge68->CalExpChargeHit(
            static_cast<float>(vr),
            static_cast<float>(angle * 180.0 / M_PI),
            static_cast<float>(vtheta * 180.0 / M_PI));

        double m_average_PE = (Evis * static_cast<double>(exp_hit_ge68)) / input.GeEvis;
        double exp_hit = m_average_PE * input.PDE / input.ESF;
        exp_hit = exp_hit * ch.rPDE + ch.dcr;
        if (exp_hit > input.saturation) exp_hit = input.saturation;
        if (exp_hit < 1e-10) exp_hit = 1e-10;

        double k = input.fChannelHit[i];
        if (k < 0) k = 0;
        likelihood += poissonNeg2LogL(k, exp_hit);
    }
    return likelihood;
}
