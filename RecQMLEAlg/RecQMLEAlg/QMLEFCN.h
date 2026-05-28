#ifndef QMLE_FCN_H
#define QMLE_FCN_H

#include <cmath>
#include <vector>
#include <array>
#include <cassert>
#include "TVector3.h"

// --- Data structures for the free-function QMLE likelihood ---

/// Per-channel data needed by the FCN (packed for cache efficiency).
struct QMLEChannelData {
    TVector3 pos;        // channel position vector (precomputed)
    double   rPDE;       // relative PDE
    double   dcr;        // dark count rate
    bool     bad;        // skip this channel if true
};

/// Input bundle for the QMLE likelihood computation.
/// All data is owned by this struct — no dependency on algorithm classes.
struct QMLEInput {
    /// Charge template (interpolated lookup: radius, theta_sipm, vtheta → expected PE)
    class ChargeTemplate* charge_template_ge68 = nullptr;

    double GeEvis = 0.9250;   // mean Qedep from detSim
    double PDE    = 1.0;      // photon detection efficiency
    double ESF    = 1.0;      // energy scale factor
    double saturation = 1.e5; // saturation threshold

    /// Measured charge per channel (8048 channels, but only meaningful if not bad)
    double fChannelHit[8048];

    /// Per-channel geometry + calibration data (length = CHANNELNUM = 8048)
    std::vector<QMLEChannelData> channels;
};

/// Compute the QMLE negative log-likelihood (-2 ln L).
///
/// Args:
///   input   — per-channel data (charge template, calibration, measured hits)
///   Evis    — visible energy (MeV)
///   vr      — vertex radius (mm)
///   vtheta  — vertex theta (radians, [0, π])
///   vphi    — vertex phi (radians, [-π, π])
///
/// Returns:
///   -2 * sum_i log(P_i), where P_i is the Poisson likelihood per channel
///
/// This is a pure function with no side effects and no external state.
/// It can be called ~10^4 times per event (inside Minuit) and must be fast.
double ComputeQMLE(const QMLEInput& input,
                   double Evis, double vr, double vtheta, double vphi);

#endif // QMLE_FCN_H