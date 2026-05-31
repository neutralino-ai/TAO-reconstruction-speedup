#include "RecQMLEAlg/QMLEFCN.h"

#include "RecQMLEAlg/ChargeTemplate.h"

#include "TVector3.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kMinExpectedHit = 1e-10;
constexpr double kMinLogProbability = -36.841361487904734;  // log(1e-16)

double poissonNeg2LogL(double observed, double expected)
{
    if (observed == 0.0) {
        return 2.0 * expected;
    }

    double log_probability = -expected + observed * std::log(expected) - std::lgamma(observed + 1.0);
    if (!std::isfinite(log_probability) || log_probability < kMinLogProbability) {
        log_probability = kMinLogProbability;
    }
    return -2.0 * log_probability;
}

double expectedHit(const QMLEInput& input, const QMLEChannelData& channel,
                   double evis, double vr, double angle, double vtheta)
{
    const float template_hit = input.charge_template_ge68->CalExpChargeHit(
        static_cast<float>(vr),
        static_cast<float>(angle * 180.0 / kPi),
        static_cast<float>(vtheta * 180.0 / kPi));

    const double average_pe = evis * static_cast<double>(template_hit) / input.GeEvis;
    double hit = average_pe * input.PDE / input.ESF;
    hit = hit * channel.rPDE + channel.dcr;
    return std::clamp(hit, kMinExpectedHit, input.saturation);
}
}  // namespace

double ComputeQMLE(const QMLEInput& input,
                   double Evis,
                   double vr,
                   double vtheta,
                   double vphi)
{
    vr = std::fabs(vr);
    TVector3 vertex(0, 0, 1);
    vertex.SetMagThetaPhi(vr, vtheta, vphi);

    double likelihood = 0.0;
    for (int channel_id = 0; channel_id < kQMLEChannelCount; ++channel_id) {
        const auto& channel = input.channels[channel_id];
        if (channel.bad) {
            continue;
        }

        const TVector3 channel_position(channel.posX, channel.posY, channel.posZ);
        const double expected = expectedHit(input, channel, Evis, vr, vertex.Angle(channel_position), vtheta);
        const double observed = std::max(input.fChannelHit[channel_id], 0.0);
        likelihood += poissonNeg2LogL(observed, expected);
    }
    return likelihood;
}
