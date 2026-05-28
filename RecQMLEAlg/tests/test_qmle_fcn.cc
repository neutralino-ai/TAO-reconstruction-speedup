// Fixture capture for QMLE FCN unit testing.
//
// This is a small standalone program that:
//   1. Constructs a QMLEInput from geometry + calibration data (same as QMLERec)
//   2. Loads an ESD file via SNiPER
//   3. Calls ComputeQMLE at known test points
//   4. Dumps the input + outputs for use as golden fixtures
//
// For now, we use a simpler approach: we hard-code a few test points
// derived from the reference reconstruction output.

#include "RecQMLEAlg/QMLEFCN.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  %-55s ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)
#define CHECK(cond) do { if (!(cond)) { FAIL(#cond); return; } } while(0)
#define CHECK_CLOSE(a, b, eps) do { \
    double _a = (a), _b = (b); \
    if (std::fabs(_a - _b) > (eps)) { \
        char buf[200]; \
        snprintf(buf, sizeof(buf), "%.10e vs %.10e (eps=%.1e)", _a, _b, (double)(eps)); \
        FAIL(buf); return; \
    } \
} while(0)

// --- Helper: build a minimal QMLEInput from a ChargeTemplate file ---

static bool load_template(const char* path, ChargeTemplate** out) {
    *out = new ChargeTemplate(std::string(path));
    return (*out)->initialize();
}

static QMLEInput make_input(ChargeTemplate* tmpl) {
    QMLEInput inp;
    memset(&inp, 0, sizeof(inp));
    inp.channels.resize(8048);
    inp.charge_template_ge68 = tmpl;
    inp.GeEvis = 0.9250;
    inp.PDE = 1.0;
    inp.ESF = 1.0;
    inp.saturation = 1e5;
    // Mark all bad by default; tests will enable specific channels
    for (auto& ch : inp.channels) ch.bad = true;
    return inp;
}


// --- Tests ---

static void test_all_bad_zero() {
    TEST("all channels bad → likelihood = 0");
    ChargeTemplate* ct = nullptr;
    if (!load_template("Ge68_charge_template", &ct)) {
        printf("SKIP (cannot load charge template)\n");
        tests_run--;
        return;
    }
    auto inp = make_input(ct);
    double r = ComputeQMLE(inp, 1.0, 100.0, 0.5, 1.2);
    CHECK_CLOSE(r, 0.0, 1e-15);
    delete ct;
    PASS();
}

static void test_one_channel_zero_hit() {
    TEST("one good channel, zero hit → compute likelihood");
    ChargeTemplate* ct = nullptr;
    if (!load_template("Ge68_charge_template", &ct)) {
        printf("SKIP (cannot load charge template)\n");
        tests_run--;
        return;
    }
    auto inp = make_input(ct);
    // Make channel 0 good, placed at +z, zero hit
    inp.channels[0].bad = false;
    inp.channels[0].pos = TVector3(0, 0, 939.515);
    inp.channels[0].rPDE = 1.0;
    inp.channels[0].dcr = 0.0;
    inp.fChannelHit[0] = 0.0;
    double r = ComputeQMLE(inp, 1.0, 100.0, 0.3, 0.0);
    CHECK(std::isfinite(r));
    CHECK(r > 0.0);
    // Also verify it's not huge (should be < 1000 for reasonable inputs)
    CHECK(r < 100.0);  
    delete ct;
    PASS();
}

static void test_monotonic_energy() {
    TEST("higher energy with same vertex → higher likelihood (worse fit)");
    ChargeTemplate* ct = nullptr;
    if (!load_template("Ge68_charge_template", &ct)) {
        printf("SKIP (cannot load charge template)\n");
        tests_run--;
        return;
    }
    auto inp = make_input(ct);
    // A few channels at different positions, all with zero hit
    for (int i = 0; i < 10; ++i) {
        inp.channels[i].bad = false;
        inp.channels[i].pos = TVector3(
            900 * std::cos(i * 0.5),
            900 * std::sin(i * 0.5),
            200.0);
        inp.channels[i].rPDE = 1.0;
        inp.channels[i].dcr = 0.0;
    }
    double r1 = ComputeQMLE(inp, 0.5, 100.0, 0.5, 0.0);
    double r2 = ComputeQMLE(inp, 1.0, 100.0, 0.5, 0.0);
    double r3 = ComputeQMLE(inp, 2.0, 100.0, 0.5, 0.0);
    CHECK(r1 > 0.0);
    CHECK(r2 > r1);  // worse fit at higher energy when hit=0
    CHECK(r3 > r2);
    delete ct;
    PASS();
}

static void test_finite_output() {
    TEST("various inputs → all finite");
    ChargeTemplate* ct = nullptr;
    if (!load_template("Ge68_charge_template", &ct)) {
        printf("SKIP (cannot load charge template)\n");
        tests_run--;
        return;
    }
    auto inp = make_input(ct);
    // Enable 100 channels with random-ish hit values
    for (int i = 0; i < 100; ++i) {
        inp.channels[i].bad = false;
        double theta = (i % 20) * 0.3;
        double phi = i * 0.6;
        inp.channels[i].pos.SetMagThetaPhi(939.515, theta, phi);
        inp.channels[i].rPDE = 0.5 + (i % 10) * 0.05;
        inp.channels[i].dcr = 0.001 * (i % 5);
        inp.fChannelHit[i] = (i % 4 == 0) ? 0.0 : (i % 3) * 1.5;
    }
    // Test a grid of vertex positions
    double E_vals[] = {0.5, 1.0, 2.0};
    double R_vals[] = {50.0, 300.0, 800.0};
    double T_vals[] = {0.2, 1.0, 2.0};
    double P_vals[] = {0.0, 1.5, 3.0};
    for (double E : E_vals)
        for (double R : R_vals)
            for (double T : T_vals)
                for (double P : P_vals) {
                    double r = ComputeQMLE(inp, E, R, T, P);
                    CHECK(std::isfinite(r));
                }
    delete ct;
    PASS();
}

int main() {
    printf("\n=== QMLE FCN Unit Tests ===\n\n");
    test_all_bad_zero();
    test_one_channel_zero_hit();
    test_monotonic_energy();
    test_finite_output();
    printf("\n%d/%d passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}