#include "RecQMLEAlg/QMLEFCN.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { tests_run++; printf("  %-55s ", n); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); } while(0)
#define CHK(c) do { if (!(c)) { FAIL(#c); return; } } while(0)
#define CHK_CLOSE(a,b,eps) do { \
    double _a=(a),_b=(b); \
    if (fabs(_a-_b)>(eps)) { char _b[200]; snprintf(_b,200,"%.10e vs %.10e",_a,_b); FAIL(_b); return; } \
} while(0)

static ChargeTemplate* load_tmpl() {
    ChargeTemplate* ct = new ChargeTemplate("Ge68_charge_template");
    if (!ct->initialize()) { delete ct; return nullptr; }
    return ct;
}

static QMLEInput make_inp(ChargeTemplate* ct) {
    QMLEInput inp; memset(&inp,0,sizeof(inp));
    inp.channels.resize(8048);
    inp.charge_template_ge68 = ct;
    inp.GeEvis=0.9250; inp.PDE=1.0; inp.ESF=1.0; inp.saturation=1e5;
    for (auto& ch : inp.channels) ch.bad = true;
    return inp;
}

// --- Tests ---

static void test_all_bad() {
    TEST("all channels bad → 0");
    auto* ct = load_tmpl(); if (!ct) { printf("SKIP\n"); tests_run--; return; }
    auto inp = make_inp(ct);
    CHK_CLOSE(ComputeQMLE(inp,1.0,100.0,0.5,1.2), 0.0, 1e-15);
    delete ct; PASS();
}

static void test_fast_matches_full() {
    TEST("ComputeQMLE_Fast == ComputeQMLE (same vertex)");
    auto* ct = load_tmpl(); if (!ct) { printf("SKIP\n"); tests_run--; return; }
    auto inp = make_inp(ct);
    for (int i = 0; i < 1000; ++i) {
        inp.channels[i].bad = false;
        double th = acos(1.0-2.0*(i+0.5)/1000.0);
        inp.channels[i].posX = 939.515*sin(th);
        inp.channels[i].posY = 0;
        inp.channels[i].posZ = 939.515*cos(th);
        inp.channels[i].rPDE = 0.8;
        inp.channels[i].dcr = 0.001*(i%5);
        inp.fChannelHit[i] = (i%4==0)?0:(i%3)*1.5;
    }
    double tests[][4] = {{0.5,100,0.5,0},{1.0,300,1.0,0},{2.0,800,2.0,0},{0.8,200,0.8,0},{1.5,500,1.2,0}};
    for (auto& t : tests) {
        QMLEPerChannelCache cache;
        PrecomputeChannelCache(inp, t[1], t[2], cache);
        double r1 = ComputeQMLE(inp, t[0], t[1], t[2], t[3]);
        double r2 = ComputeQMLE_Fast(inp, cache, t[0], t[3]);
        CHK_CLOSE(r1, r2, 1e-10);
    }
    delete ct; PASS();
}

static void test_monotonic() {
    TEST("higher Evis → higher likelihood (worse)");
    auto* ct = load_tmpl(); if (!ct) { printf("SKIP\n"); tests_run--; return; }
    auto inp = make_inp(ct);
    for (int i=0;i<500;++i){inp.channels[i].bad=false;inp.channels[i].posX=900*cos(i*0.1);inp.channels[i].posY=900*sin(i*0.1);inp.channels[i].posZ=200;inp.channels[i].rPDE=1.0;inp.channels[i].dcr=0.001;}
    QMLEPerChannelCache cache;
    PrecomputeChannelCache(inp, 100.0, 0.5, cache);
    double r1=ComputeQMLE_Fast(inp,cache,0.5,0);
    double r2=ComputeQMLE_Fast(inp,cache,1.0,0);
    double r3=ComputeQMLE_Fast(inp,cache,2.0,0);
    CHK(r1>0.0); CHK(r2>r1); CHK(r3>r2);
    delete ct; PASS();
}

static void test_finite() {
    TEST("grid search → all finite (fast)");
    auto* ct = load_tmpl(); if (!ct) { printf("SKIP\n"); tests_run--; return; }
    auto inp = make_inp(ct);
    for (int i=0;i<1000;++i){inp.channels[i].bad=false;inp.channels[i].posX=900*cos(i*0.1);inp.channels[i].posY=900*sin(i*0.1);inp.channels[i].posZ=200;inp.channels[i].rPDE=0.8;inp.channels[i].dcr=0.001*(i%5);inp.fChannelHit[i]=(i%4==0)?0:(i%3)*1.5;}
    double Evals[]={0.5,1.0,2.0},Rvals[]={50,300,800},Tvals[]={0.2,1.0,2.0};
    for (double R:Rvals) for (double T:Tvals) {
        QMLEPerChannelCache cache;
        PrecomputeChannelCache(inp,R,T,cache);
        for (double E:Evals) {
            double r=ComputeQMLE_Fast(inp,cache,E,0);
            CHK(std::isfinite(r));
        }
    }
    delete ct; PASS();
}

int main() {
    printf("\n=== QMLE FCN Unit Tests (with fast path) ===\n\n");
    test_all_bad();
    test_fast_matches_full();
    test_monotonic();
    test_finite();
    printf("\n%d/%d passed\n", tests_passed, tests_run);
    return tests_passed==tests_run?0:1;
}
