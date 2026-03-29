#include "core/compact_fs_tree.h"
#include "core/node.h"
#include "builder/fs_tree_builder.h"

#include <cassert>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static CompactFSTree build_simple_tree(CaseSensitivity cs = CaseSensitivity::Sensitive) {
    FSTraits tr{cs, NormForm::None, 1, true};
    FSTreeBuilder b(tr);

    //  root/
    //    a/
    //      aa
    //      ab
    //    b/
    //      ba
    //    c
    uint32_t root = b.add_entry(UINT32_MAX, "root", NODE_TYPE_DIR, 0, 0);
    uint32_t a    = b.add_entry(root, "a",  NODE_TYPE_DIR,  0,    1000);
    uint32_t aa   = b.add_entry(a,    "aa", NODE_TYPE_FILE, 100,  1001);
    uint32_t ab   = b.add_entry(a,    "ab", NODE_TYPE_FILE, 200,  1002);
    uint32_t bdir = b.add_entry(root, "b",  NODE_TYPE_DIR,  0,    1003);
    uint32_t ba   = b.add_entry(bdir, "ba", NODE_TYPE_FILE, 300,  1004);
    uint32_t c    = b.add_entry(root, "c",  NODE_TYPE_FILE, 400,  1005);

    (void)aa; (void)ab; (void)ba; (void)c;
    return b.build();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_size() {
    auto tree = build_simple_tree();
    assert(tree.size() == 7);
    std::cout << "PASS test_size\n";
}

static void test_resolve_root() {
    auto tree = build_simple_tree();
    auto r = tree.resolve("");
    assert(r.has_value());
    assert(*r == 0);
    assert(tree.name(0) == "root");
    std::cout << "PASS test_resolve_root\n";
}

static void test_resolve_path() {
    auto tree = build_simple_tree();

    auto r = tree.resolve("a/aa");
    assert(r.has_value());
    assert(tree.name(*r) == "aa");

    r = tree.resolve("b/ba");
    assert(r.has_value());
    assert(tree.name(*r) == "ba");

    r = tree.resolve("c");
    assert(r.has_value());
    assert(tree.name(*r) == "c");

    std::cout << "PASS test_resolve_path\n";
}

static void test_resolve_missing() {
    auto tree = build_simple_tree();
    assert(!tree.resolve("a/zz").has_value());
    assert(!tree.resolve("x").has_value());
    std::cout << "PASS test_resolve_missing\n";
}

static void test_children_contiguous() {
    auto tree = build_simple_tree();
    // Root should have 3 children (a, b, c) sorted alphabetically.
    auto ch = tree.children(0);
    assert(ch.size() == 3);
    assert(tree.name(tree.node(0).children_begin + 0) == "a");
    assert(tree.name(tree.node(0).children_begin + 1) == "b");
    assert(tree.name(tree.node(0).children_begin + 2) == "c");
    std::cout << "PASS test_children_contiguous\n";
}

static void test_full_path() {
    auto tree = build_simple_tree();
    // resolve followed by full_path must be identity.
    auto r = tree.resolve("a/aa");
    assert(r.has_value());
    std::string fp = tree.full_path(*r);
    assert(fp == "a/aa" || fp == "root/a/aa"); // depends on whether root name is included
    // Verify round-trip via resolve (strip root segment if present).
    auto r2 = tree.resolve(fp);
    assert(r2.has_value());
    assert(*r2 == *r);
    std::cout << "PASS test_full_path\n";
}

static void test_parent_chain() {
    auto tree = build_simple_tree();
    auto r = tree.resolve("a/aa");
    assert(r.has_value());
    uint32_t aa_idx = *r;
    uint32_t a_idx  = tree.parent(aa_idx);
    uint32_t root_idx = tree.parent(a_idx);
    assert(tree.parent(root_idx) == UINT32_MAX);
    assert(tree.name(a_idx)    == "a");
    assert(tree.name(root_idx) == "root");
    std::cout << "PASS test_parent_chain\n";
}

static void test_case_insensitive() {
    auto tree = build_simple_tree(CaseSensitivity::Insensitive);
    auto r = tree.resolve("A/AA");
    assert(r.has_value());
    assert(tree.name(*r) == "aa"); // stored as lowercase after normalization?
    // Actually FATScanner normalizes to lower; here we just built with lowercase names.
    // Binary search should find it case-insensitively.
    std::cout << "PASS test_case_insensitive\n";
}

static void test_visit_subtree_count() {
    auto tree = build_simple_tree();
    uint32_t count = 0;
    tree.visit_subtree(0, [&](uint32_t) { ++count; });
    assert(count == 7);
    std::cout << "PASS test_visit_subtree_count\n";
}

static void test_file_size_mtime() {
    auto tree = build_simple_tree();
    auto r = tree.resolve("b/ba");
    assert(r.has_value());
    const Node& n = tree.node(*r);
    assert(n.file_size == 300);
    assert(n.mtime     == 1004);
    std::cout << "PASS test_file_size_mtime\n";
}

// ---------------------------------------------------------------------------

int main() {
    test_size();
    test_resolve_root();
    test_resolve_path();
    test_resolve_missing();
    test_children_contiguous();
    test_full_path();
    test_parent_chain();
    test_case_insensitive();
    test_visit_subtree_count();
    test_file_size_mtime();

    std::cout << "\nAll CompactFSTree tests passed.\n";
    return 0;
}
