#include "builder/fs_tree_builder.h"
#include "core/compact_fs_tree.h"
#include "core/node.h"
#include "serialization/fs_tree_serializer.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Synthetic tree generator
// ---------------------------------------------------------------------------

struct GenParams {
    uint32_t total_nodes     = 1'000'000;
    uint32_t max_depth       = 15;
    uint32_t normal_fanout   = 50;  // max fanout for regular dirs
    uint32_t big_fanout      = 500; // fanout for "big" dirs
    double   big_dir_prob    = 0.02;
    uint32_t min_name_len    = 8;
    uint32_t max_name_len    = 32;
};

static std::string random_name(std::mt19937& rng, uint32_t min_len, uint32_t max_len) {
    static const char chars[] =
        "abcdefghijklmnopqrstuvwxyz0123456789_-";
    std::uniform_int_distribution<uint32_t> len_dist(min_len, max_len);
    std::uniform_int_distribution<uint32_t> char_dist(0, sizeof(chars) - 2);
    uint32_t len = len_dist(rng);
    std::string s;
    s.reserve(len);
    for (uint32_t i = 0; i < len; ++i)
        s += chars[char_dist(rng)];
    return s;
}

static void gen_recursive(FSTreeBuilder& builder,
                           std::mt19937& rng,
                           uint32_t parent,
                           uint32_t depth,
                           uint32_t& remaining,
                           const GenParams& p) {
    if (remaining == 0) return;

    std::bernoulli_distribution big_dir_dist(p.big_dir_prob);
    uint32_t fanout_limit = big_dir_dist(rng) ? p.big_fanout : p.normal_fanout;
    std::uniform_int_distribution<uint32_t> fanout_dist(1, fanout_limit);
    uint32_t fanout = std::min(fanout_dist(rng), remaining);

    // Log-normal file size: exp(normal(10, 3)) → median ~22 KB
    std::normal_distribution<double> size_dist(10.8, 2.0);
    std::uniform_int_distribution<uint64_t> mtime_dist(0, 1'700'000'000ULL * 1'000'000'000ULL);

    for (uint32_t i = 0; i < fanout && remaining > 0; ++i) {
        std::string name = random_name(rng, p.min_name_len, p.max_name_len);
        bool is_dir = (depth < p.max_depth) && (remaining > 1) &&
                      std::bernoulli_distribution(0.3)(rng);

        uint64_t sz = is_dir ? 0 : static_cast<uint64_t>(std::exp(size_dist(rng)));
        uint64_t mt = mtime_dist(rng);

        uint32_t id = builder.add_entry(parent,
                                         std::move(name),
                                         is_dir ? NODE_TYPE_DIR : NODE_TYPE_FILE,
                                         sz,
                                         mt);
        --remaining;

        if (is_dir) {
            gen_recursive(builder, rng, id, depth + 1, remaining, p);
        }
    }
}

static CompactFSTree generate_tree(const GenParams& p, uint32_t seed = 42) {
    FSTraits tr{CaseSensitivity::Sensitive, NormForm::None, 1, true};
    FSTreeBuilder builder(tr);

    uint32_t root = builder.add_entry(UINT32_MAX, "synthroot", NODE_TYPE_DIR, 0, 0);
    uint32_t remaining = p.total_nodes - 1;

    std::mt19937 rng(seed);
    gen_recursive(builder, rng, root, 1, remaining, p);

    return builder.build();
}

// ---------------------------------------------------------------------------
// Benchmark helpers
// ---------------------------------------------------------------------------

using Clock = std::chrono::high_resolution_clock;

static double elapsed_ms(Clock::time_point start) {
    auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static void benchmark_build(uint32_t node_count) {
    GenParams p;
    p.total_nodes = node_count;

    auto t0 = Clock::now();
    CompactFSTree tree = generate_tree(p);
    double ms = elapsed_ms(t0);

    std::cout << "build(" << node_count << " nodes): "
              << ms << " ms  (" << tree.size() << " actual)\n";
}

static void benchmark_resolve(const CompactFSTree& tree, uint32_t samples) {
    // Collect sample paths.
    std::vector<std::string> paths;
    paths.reserve(samples);
    std::mt19937 rng(123);
    std::uniform_int_distribution<uint32_t> dist(0, tree.size() - 1);
    for (uint32_t i = 0; i < samples; ++i)
        paths.push_back(tree.full_path(dist(rng)));

    auto t0 = Clock::now();
    uint32_t found = 0;
    for (const auto& p : paths)
        if (tree.resolve(p)) ++found;
    double ms = elapsed_ms(t0);

    std::cout << "resolve(" << samples << " paths): "
              << ms << " ms  ("
              << (ms * 1e3 / samples) << " µs/path, "
              << found << " found)\n";
}

static void benchmark_traverse(const CompactFSTree& tree) {
    auto t0 = Clock::now();
    uint64_t sum = 0;
    tree.visit_subtree(0, [&](uint32_t id) { sum += tree.node(id).file_size; });
    double ms = elapsed_ms(t0);
    std::cout << "full DFS traverse(" << tree.size() << " nodes): "
              << ms << " ms  (sum=" << sum << ")\n";
}

// ---------------------------------------------------------------------------
// Correctness: resolve all inserted paths
// ---------------------------------------------------------------------------

static void test_resolve_roundtrip(uint32_t node_count) {
    GenParams p;
    p.total_nodes = node_count;
    CompactFSTree tree = generate_tree(p, 7);

    uint32_t fails = 0;
    for (uint32_t id = 0; id < tree.size(); ++id) {
        std::string fp = tree.full_path(id);
        auto r = tree.resolve(fp);
        if (!r.has_value() || *r != id) {
            ++fails;
            if (fails <= 5)
                std::cerr << "FAIL resolve(" << fp << ") -> "
                          << (r.has_value() ? std::to_string(*r) : "nullopt")
                          << " expected " << id << "\n";
        }
    }
    if (fails == 0)
        std::cout << "PASS resolve_roundtrip(" << node_count << " nodes)\n";
    else
        std::cout << "FAIL resolve_roundtrip: " << fails << " failures\n";
}

int main() {
    std::cout << "=== Correctness ===\n";
    test_resolve_roundtrip(10'000);

    std::cout << "\n=== Benchmarks ===\n";
    benchmark_build(100'000);
    benchmark_build(1'000'000);

    GenParams p;
    p.total_nodes = 1'000'000;
    CompactFSTree tree = generate_tree(p);
    benchmark_resolve(tree, 100'000);
    benchmark_traverse(tree);

    return 0;
}
