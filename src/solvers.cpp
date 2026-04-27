#include "solvers.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <random>

static double dist2(double ax, double ay, double bx, double by) {
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx*dx + dy*dy;
}

CoverageStats compute_coverage(const std::vector<DemandPoint>& pts, const std::vector<AccessPoint>& aps, double radius_m) {
    CoverageStats st;
    st.total_points = pts.size();
    if (pts.empty()) return st;

    const double r2 = radius_m * radius_m;
    std::size_t covered = 0;
    for (const auto& p : pts) {
        bool ok = false;
        for (const auto& ap : aps) {
            if (dist2(p.x_m, p.y_m, ap.x_m, ap.y_m) <= r2) {
                ok = true;
                break;
            }
        }
        if (ok) covered++;
    }
    st.covered_points = covered;
    st.coverage_percent = 100.0 * static_cast<double>(covered) / static_cast<double>(pts.size());
    return st;
}

Solution solve_none(const std::vector<DemandPoint>& pts, double radius_m) {
    Solution s;
    s.solver_name = "none";
    s.radius_m = radius_m;
    s.aps.clear();
    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

Solution solve_one_at_first(const std::vector<DemandPoint>& pts, double radius_m) {
    Solution s;
    s.solver_name = "one_at_first";
    s.radius_m = radius_m;

    if (!pts.empty()) {
        AccessPoint ap;
        ap.label = pts[0].name.empty() ? pts[0].id : pts[0].name;
        ap.x_m = pts[0].x_m;
        ap.y_m = pts[0].y_m;
        s.aps.push_back(ap);
    }

    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

Solution solve_random_k(const std::vector<DemandPoint>& pts, double radius_m, std::size_t k, std::uint64_t seed) {
    Solution s;
    s.solver_name = "random_k";
    s.radius_m = radius_m;

    if (pts.empty() || k == 0) {
        s.stats = compute_coverage(pts, s.aps, radius_m);
        return s;
    }

    std::mt19937_64 rng(seed);
    std::vector<std::size_t> idx(pts.size());
    for (std::size_t i = 0; i < pts.size(); i++) idx[i] = i;
    std::shuffle(idx.begin(), idx.end(), rng);

    if (k > idx.size()) k = idx.size();
    s.aps.reserve(k);
    for (std::size_t i = 0; i < k; i++) {
        const auto& p = pts[idx[i]];
        AccessPoint ap;
        ap.label = p.name.empty() ? p.id : p.name;
        ap.x_m = p.x_m;
        ap.y_m = p.y_m;
        s.aps.push_back(ap);
    }

    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

// ── Brute-force ──────────────────────────────────────────────────────────────
// Enumerates all subsets of demand-point positions as candidate AP placements.
// Only feasible for small n (≤ ~20). Respects timeout_ms; returns best found.
Solution solve_brute(const std::vector<DemandPoint>& pts, double radius_m, int timeout_ms, int max_aps) {
    Solution s;
    s.solver_name = "brute";
    s.radius_m = radius_m;

    const int n = static_cast<int>(pts.size());
    if (n == 0) { s.stats = compute_coverage(pts, s.aps, radius_m); return s; }

    const double r2 = radius_m * radius_m;
    const auto t_start = std::chrono::steady_clock::now();

    // best starts as "place one AP at every point"
    int best_count = (max_aps > 0 && max_aps < n) ? max_aps + 1 : n;
    int best_mask  = (1 << n) - 1;

    for (int mask = 1; mask < (1 << n); ++mask) {
        // prune by time
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - t_start).count() >= timeout_ms)
            break;

        int used = __builtin_popcount(mask);
        if (used >= best_count) continue;
        if (max_aps > 0 && used > max_aps) continue;

        bool ok = true;
        for (int i = 0; i < n && ok; ++i) {
            bool covered = false;
            for (int j = 0; j < n && !covered; ++j) {
                if (!(mask & (1 << j))) continue;
                double dx = pts[j].x_m - pts[i].x_m;
                double dy = pts[j].y_m - pts[i].y_m;
                if (dx*dx + dy*dy <= r2) covered = true;
            }
            if (!covered) ok = false;
        }
        if (ok) { best_count = used; best_mask = mask; }
    }

    for (int j = 0; j < n; ++j) {
        if (!(best_mask & (1 << j))) continue;
        AccessPoint ap;
        ap.label = pts[j].name.empty() ? pts[j].id : pts[j].name;
        ap.x_m = pts[j].x_m;
        ap.y_m = pts[j].y_m;
        s.aps.push_back(ap);
    }
    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

// ── Greedy 1: Maximum coverage ────────────────────────────────────────────────
// Each round: place AP at the demand-point location that covers the most
// currently-uncovered points. Stops when all covered or max_aps reached.
Solution solve_greedy1(const std::vector<DemandPoint>& pts, double radius_m, int max_aps) {
    Solution s;
    s.solver_name = "greedy1";
    s.radius_m = radius_m;

    const int n = static_cast<int>(pts.size());
    if (n == 0) { s.stats = compute_coverage(pts, s.aps, radius_m); return s; }

    const double r2 = radius_m * radius_m;
    std::vector<bool> covered(n, false);
    int remaining = n;

    while (remaining > 0) {
        if (max_aps > 0 && static_cast<int>(s.aps.size()) >= max_aps) break;

        int best_idx = -1, best_gain = -1;
        for (int j = 0; j < n; ++j) {
            int gain = 0;
            for (int i = 0; i < n; ++i) {
                if (covered[i]) continue;
                double dx = pts[j].x_m - pts[i].x_m;
                double dy = pts[j].y_m - pts[i].y_m;
                if (dx*dx + dy*dy <= r2) ++gain;
            }
            if (gain > best_gain) { best_gain = gain; best_idx = j; }
        }
        if (best_idx < 0 || best_gain == 0) break;

        AccessPoint ap;
        ap.label = pts[best_idx].name.empty() ? pts[best_idx].id : pts[best_idx].name;
        ap.x_m = pts[best_idx].x_m;
        ap.y_m = pts[best_idx].y_m;
        s.aps.push_back(ap);

        for (int i = 0; i < n; ++i) {
            if (covered[i]) continue;
            double dx = ap.x_m - pts[i].x_m;
            double dy = ap.y_m - pts[i].y_m;
            if (dx*dx + dy*dy <= r2) { covered[i] = true; --remaining; }
        }
    }

    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

// ── Greedy 2: Farthest-uncovered-first ────────────────────────────────────────
// Each round: find the uncovered point farthest from any existing AP (or the
// global centroid if no APs yet), then place an AP there.
Solution solve_greedy2(const std::vector<DemandPoint>& pts, double radius_m, int max_aps) {
    Solution s;
    s.solver_name = "greedy2";
    s.radius_m = radius_m;

    const int n = static_cast<int>(pts.size());
    if (n == 0) { s.stats = compute_coverage(pts, s.aps, radius_m); return s; }

    const double r2 = radius_m * radius_m;
    std::vector<bool> covered(n, false);
    int remaining = n;

    while (remaining > 0) {
        if (max_aps > 0 && static_cast<int>(s.aps.size()) >= max_aps) break;

        int farthest_idx = -1;
        double farthest_dist2 = -1.0;

        for (int i = 0; i < n; ++i) {
            if (covered[i]) continue;
            double min_d2 = std::numeric_limits<double>::max();
            if (s.aps.empty()) {
                min_d2 = 0.0; // treat all as equally "far" — pick first uncovered
            } else {
                for (const auto& ap : s.aps) {
                    double dx = pts[i].x_m - ap.x_m;
                    double dy = pts[i].y_m - ap.y_m;
                    double d2 = dx*dx + dy*dy;
                    if (d2 < min_d2) min_d2 = d2;
                }
            }
            if (min_d2 > farthest_dist2) { farthest_dist2 = min_d2; farthest_idx = i; }
        }
        if (farthest_idx < 0) break;

        AccessPoint ap;
        ap.label = pts[farthest_idx].name.empty() ? pts[farthest_idx].id : pts[farthest_idx].name;
        ap.x_m = pts[farthest_idx].x_m;
        ap.y_m = pts[farthest_idx].y_m;
        s.aps.push_back(ap);

        for (int i = 0; i < n; ++i) {
            if (covered[i]) continue;
            double dx = ap.x_m - pts[i].x_m;
            double dy = ap.y_m - pts[i].y_m;
            if (dx*dx + dy*dy <= r2) { covered[i] = true; --remaining; }
        }
    }

    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

// ── Greedy 3: Centroid-of-uncovered ──────────────────────────────────────────
// Each round: compute the centroid of all currently-uncovered points and place
// an AP there. The centroid minimises total squared distance to uncovered pts.
Solution solve_greedy3(const std::vector<DemandPoint>& pts, double radius_m, int max_aps) {
    Solution s;
    s.solver_name = "greedy3";
    s.radius_m = radius_m;

    const int n = static_cast<int>(pts.size());
    if (n == 0) { s.stats = compute_coverage(pts, s.aps, radius_m); return s; }

    const double r2 = radius_m * radius_m;
    std::vector<bool> covered(n, false);
    int remaining = n;

    while (remaining > 0) {
        if (max_aps > 0 && static_cast<int>(s.aps.size()) >= max_aps) break;

        double cx = 0.0, cy = 0.0;
        int cnt = 0;
        for (int i = 0; i < n; ++i) {
            if (!covered[i]) { cx += pts[i].x_m; cy += pts[i].y_m; ++cnt; }
        }
        if (cnt == 0) break;
        cx /= cnt; cy /= cnt;

        // Snap to the uncovered demand point nearest the centroid so the AP
        // is guaranteed to sit on a real location and cover nearby buildings.
        int snap_idx = -1;
        double snap_d2 = std::numeric_limits<double>::max();
        for (int i = 0; i < n; ++i) {
            if (covered[i]) continue;
            double dx = pts[i].x_m - cx, dy = pts[i].y_m - cy;
            double d2 = dx*dx + dy*dy;
            if (d2 < snap_d2) { snap_d2 = d2; snap_idx = i; }
        }
        if (snap_idx < 0) break;

        AccessPoint ap;
        ap.label = pts[snap_idx].name.empty() ? pts[snap_idx].id : pts[snap_idx].name;
        ap.x_m = pts[snap_idx].x_m;
        ap.y_m = pts[snap_idx].y_m;
        s.aps.push_back(ap);

        for (int i = 0; i < n; ++i) {
            if (covered[i]) continue;
            double dx = ap.x_m - pts[i].x_m;
            double dy = ap.y_m - pts[i].y_m;
            if (dx*dx + dy*dy <= r2) { covered[i] = true; --remaining; }
        }
    }

    s.stats = compute_coverage(pts, s.aps, radius_m);
    return s;
}

std::vector<std::string> available_solvers() {
    return {"none", "one_at_first", "random_k", "brute", "greedy1", "greedy2", "greedy3"};
}
