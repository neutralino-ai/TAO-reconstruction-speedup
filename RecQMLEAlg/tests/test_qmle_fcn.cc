#include "RecQMLEAlg/ChargeTemplate.h"
#include "RecQMLEAlg/QMLEFCN.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

constexpr int kChannelCount = 8048;
constexpr double kRelativeTolerance = 1e-13;

struct Fixture {
    double evis = 0.0;
    double vr = 0.0;
    double vtheta = 0.0;
    double vphi = 0.0;
    double fcn = 0.0;
    int active_channels = 0;
    double fChannelTotHit = 0.0;
    QMLEInput input;
};

double parse_value(const std::string& line, const std::string& key)
{
    const std::string prefix = key + "=";
    if (line.rfind(prefix, 0) != 0) {
        throw std::runtime_error("expected key " + key + ", got: " + line);
    }
    return std::stod(line.substr(prefix.size()));
}

Fixture load_fixture(const std::string& path, ChargeTemplate* charge_template)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("failed to open fixture: " + path);
    }

    Fixture fx;
    fx.input.channels.resize(kChannelCount);
    fx.input.charge_template_ge68 = charge_template;

    std::string line;
    std::getline(in, line);
    fx.evis = parse_value(line, "Evis");
    std::getline(in, line);
    fx.vr = parse_value(line, "vr");
    std::getline(in, line);
    fx.vtheta = parse_value(line, "vtheta");
    std::getline(in, line);
    fx.vphi = parse_value(line, "vphi");
    std::getline(in, line);
    fx.input.GeEvis = parse_value(line, "GeEvis");
    std::getline(in, line);
    fx.input.PDE = parse_value(line, "PDE");
    std::getline(in, line);
    fx.input.ESF = parse_value(line, "ESF");
    std::getline(in, line);
    fx.input.saturation = parse_value(line, "saturation");
    std::getline(in, line);
    fx.active_channels = static_cast<int>(parse_value(line, "active_channels"));
    std::getline(in, line);
    fx.fChannelTotHit = parse_value(line, "fChannelTotHit");

    int counted_active = 0;
    double counted_hit = 0.0;
    for (int row = 0; row < kChannelCount; ++row) {
        if (!std::getline(in, line)) {
            throw std::runtime_error("fixture ended while reading channel rows");
        }
        std::istringstream ss(line);
        int id = -1;
        int bad = 1;
        double hit = 0.0;
        double rpde = 0.0;
        double dcr = 0.0;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        if (!(ss >> id >> hit >> bad >> rpde >> dcr >> x >> y >> z)) {
            throw std::runtime_error("bad channel row: " + line);
        }
        if (id != row) {
            throw std::runtime_error("unexpected channel id in fixture");
        }
        fx.input.fChannelHit[id] = hit;
        fx.input.channels[id].bad = bad != 0;
        fx.input.channels[id].rPDE = rpde;
        fx.input.channels[id].dcr = dcr;
        fx.input.channels[id].posX = x;
        fx.input.channels[id].posY = y;
        fx.input.channels[id].posZ = z;
        if (bad == 0) {
            ++counted_active;
        }
        counted_hit += hit;
    }

    std::getline(in, line);
    if (line != "END_CH") {
        throw std::runtime_error("expected END_CH, got: " + line);
    }
    std::getline(in, line);
    fx.fcn = parse_value(line, "FCN");

    if (counted_active != fx.active_channels) {
        throw std::runtime_error("active channel count mismatch");
    }
    if (std::fabs(counted_hit - fx.fChannelTotHit) > 1e-9) {
        throw std::runtime_error("total hit count mismatch");
    }
    if (fx.active_channels <= 0 || fx.fcn == 0.0) {
        throw std::runtime_error("fixture is degenerate");
    }

    return fx;
}

}  // namespace

int main()
{
    try {
        ChargeTemplate charge_template("Ge68_charge_template");
        if (!charge_template.initialize()) {
            throw std::runtime_error("failed to initialize charge template");
        }

        const Fixture fixture = load_fixture("fixtures/fcn_evt0.txt", &charge_template);
        const double computed = ComputeQMLE(
            fixture.input, fixture.evis, fixture.vr, fixture.vtheta, fixture.vphi);
        const double abs_diff = std::fabs(computed - fixture.fcn);
        const double rel_diff = abs_diff / std::fabs(fixture.fcn);

        std::cout.precision(17);
        std::cout << "fixture FCN:  " << fixture.fcn << "\n";
        std::cout << "computed FCN: " << computed << "\n";
        std::cout << "abs diff:     " << abs_diff << "\n";
        std::cout << "rel diff:     " << rel_diff << "\n";

        if (rel_diff > kRelativeTolerance) {
            throw std::runtime_error("fixture FCN mismatch");
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }

    return 0;
}
