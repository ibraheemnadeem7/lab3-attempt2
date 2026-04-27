#include "csv.h"
#include "solvers.h"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using SteadyClock = std::chrono::steady_clock;

struct Options {
    std::string in_csv;
    double radius_m = 50.0;
    std::string solver = "greedy1";
    std::size_t k = 5;
    std::uint64_t seed = 12345;
    int max_aps = -1;
    int timeout_ms = 5000;
};

static void print_usage(const char* argv0) {
    std::cout
        << "Usage:\n"
        << "  " << argv0 << " <clean_points.csv> --radius <meters> --solver <name> [--k <int>] [--seed <int>]\n\n"
        << "Input CSV format (from osm_wifi_prep):\n"
        << "  id,name,lat,lon,x_m,y_m\n\n"
        << "Solvers:\n";
    for (const auto& s : available_solvers()) {
        std::cout << "  - " << s << "\n";
    }
    std::cout
        << "\nExamples:\n"
        << "  " << argv0 << " kenyonout.csv --radius 60 --solver one_at_first\n"
        << "  " << argv0 << " kenyonout.csv --radius 80 --solver random_k --k 10 --seed 42\n";
}

static Options parse_args(int argc, char** argv) {
    if (argc < 2) {
        throw std::runtime_error("Missing input CSV");
    }

    Options opt;
    opt.in_csv = argv[1];

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        auto need = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("Missing value for ") + flag);
            return argv[++i];
        };

        if (a == "--radius") {
            opt.radius_m = std::stod(need("--radius"));
        } else if (a == "--solver" || a == "--algorithm") {
            opt.solver = need(a.c_str());
        } else if (a == "--k") {
            opt.k = static_cast<std::size_t>(std::stoull(need("--k")));
        } else if (a == "--seed") {
            opt.seed = static_cast<std::uint64_t>(std::stoull(need("--seed")));
        } else if (a == "--max-aps") {
            opt.max_aps = std::stoi(need("--max-aps"));
        } else if (a == "--timeout-ms") {
            opt.timeout_ms = std::stoi(need("--timeout-ms"));
        } else if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + a);
        }
    }

    if (opt.radius_m <= 0.0) throw std::runtime_error("radius must be > 0");
    return opt;
}

static Solution run_solver(const std::vector<DemandPoint>& pts, const Options& opt) {
    if (opt.solver == "none")         return solve_none(pts, opt.radius_m);
    if (opt.solver == "one_at_first") return solve_one_at_first(pts, opt.radius_m);
    if (opt.solver == "random_k")     return solve_random_k(pts, opt.radius_m, opt.k, opt.seed);
    if (opt.solver == "brute")        return solve_brute(pts, opt.radius_m, opt.timeout_ms, opt.max_aps);
    if (opt.solver == "greedy1")      return solve_greedy1(pts, opt.radius_m, opt.max_aps);
    if (opt.solver == "greedy2")      return solve_greedy2(pts, opt.radius_m, opt.max_aps);
    if (opt.solver == "greedy3")      return solve_greedy3(pts, opt.radius_m, opt.max_aps);
    throw std::runtime_error("Unknown solver name: " + opt.solver);
}

static void report_solution(const std::vector<DemandPoint>& pts, const Solution& sol, double solve_ms) {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Points:    " << pts.size() << "\n";
    std::cout << "Radius:    " << sol.radius_m << " m\n";
    std::cout << "Algorithm: " << sol.solver_name << "\n";
    std::cout << "APs placed: " << sol.aps.size() << "\n";
    std::cout << "Coverage:  " << sol.stats.coverage_percent << "%"
              << " (" << sol.stats.covered_points << "/" << sol.stats.total_points << ")\n";
    std::cout << "Runtime:   " << std::setprecision(1) << solve_ms << " ms\n";
}

int main(int argc, char** argv) {
    try {
        Options opt = parse_args(argc, argv);

        const auto pts = load_demand_points_csv(opt.in_csv);

        auto t2 = SteadyClock::now();
        const auto sol = run_solver(pts, opt);
        auto t3 = SteadyClock::now();
        const auto solve_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();

        report_solution(pts, sol, solve_ms);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Run with --help for usage.\n";
        return 1;
    }
}
