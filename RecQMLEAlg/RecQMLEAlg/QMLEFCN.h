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
    double posX, posY, posZ;    // precomputed position (flat x,y,z, no TVector3 overhead)
    double rPDE;                 // relative PDE
    double dcr;                  // dark count rate
    bool   bad;                  // skip if true
};

/// Input bundle for the QMLE likelihood computation.
struct QMLEInput {
    /// Charge template (interpolated lookup: radius, theta_sipm, vtheta → expected PE)
    class ChargeTemplate* charge_template_ge68 = nullptr;

    double GeEvis = 0.9250;   // mean Qedep from detSim
    double PDE    = 1.0;      // photon detection efficiency
    double ESF    = 1.0;      // energy scale factor
    double saturation = 1.e5; // saturation threshold

    /// Measured charge per channel (8048 channels)
    double fChannelHit[8048];

    /// Per-channel geometry (length = 8048)
    std::vector<QMLEChannelData> channels;
};

/// Precomputed per-channel values for a given vertex position.
/// These are the parts of the expected charge that do NOT depend on Evis.
struct QMLEPerChannelCache {
    static constexpr int NMAX = 8048;
    double expHitPerUnitEnergy[NMAX]; // expected hit / MeV from template+geometry
    int nActive;                       // number of non-bad channels
    int activeIndices[NMAX];          // indices of active channels (cache-friendly iteration)
};

/// Precompute per-channel values for a specific vertex (vr, vtheta).
/// This should be called once per Minuit iteration, before looping over Evis.
void PrecomputeChannelCache(const QMLEInput& input,
                            double vr, double vtheta,
                            QMLEPerChannelCache& cache);

/// Compute the QMLE negative log-likelihood (-2 ln L) using precomputed cache.
/// MUCH faster than the full ComputeQMLE because it skips geometry+template work.
///
/// Args:
///   input   — per-channel data
///   cache   — precomputed by PrecomputeChannelCache
///   Evis    — visible energy (MeV)  [CHANGES each Minuit iteration]
///   vphi    — vertex phi (radians)  [CHANGES each Minuit iteration]
///
/// Note: vr and vtheta are baked into the cache; only Evis and vphi vary.
double ComputeQMLE_Fast(const QMLEInput& input,
                        const QMLEPerChannelCache& cache,
                        double Evis, double vphi);

/// Original full compute (kept for validation / reference).
double ComputeQMLE(const QMLEInput& input,
                   double Evis, double vr, double vtheta, double vphi);

#endif // QMLE_FCN_H
