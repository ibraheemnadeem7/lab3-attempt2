#pragma once

#include "solution.h"

#include <cstdint>
#include <string>
#include <vector>

Solution solve_none(const std::vector<DemandPoint>& pts, double radius_m);
Solution solve_one_at_first(const std::vector<DemandPoint>& pts, double radius_m);
Solution solve_random_k(const std::vector<DemandPoint>& pts, double radius_m, std::size_t k, std::uint64_t seed);

Solution solve_brute(const std::vector<DemandPoint>& pts, double radius_m, int timeout_ms = 5000, int max_aps = -1);
Solution solve_greedy1(const std::vector<DemandPoint>& pts, double radius_m, int max_aps = -1);
Solution solve_greedy2(const std::vector<DemandPoint>& pts, double radius_m, int max_aps = -1);
Solution solve_greedy3(const std::vector<DemandPoint>& pts, double radius_m, int max_aps = -1);

CoverageStats compute_coverage(const std::vector<DemandPoint>& pts, const std::vector<AccessPoint>& aps, double radius_m);

std::vector<std::string> available_solvers();
