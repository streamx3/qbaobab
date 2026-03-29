#include "builder/fs_tree_builder.h"
#include "core/compact_fs_tree.h"
#include "core/node.h"

#include <cassert>
#include <iostream>
#include <string>

// Verify that children are sorted after build().
static void test_children_sorted() {
    FSTraits tr{CaseSensitivity::Sensitive, NormForm::None, 1, true};
    FSTreeBuilder b(tr);

    uint32_t root = b.add_entry(UINT32_MAX, "root", NODE_TYPE_DIR, 0, 0);
    // Add in reverse alphabetical order.
    b.add_entry(root, "z", NODE_TYPE_FILE, 1, 0);
    b.add_entry(root, "a", NODE_TYPE_FILE, 2, 0);
    b.add_entry(root, "m", NODE_TYPE_FILE, 3, 0);

    CompactFSTree tree = b.build();
    assert(tree.size() == 4);

    auto ch = tree.children(0);
    assert(ch.size() == 3);
    assert(tree.name(tree.node(0).children_begin + 0) == "a");
    assert(tree.name(tree.node(0).children_begin + 1) == "m");
    assert(tree.name(tree.node(0).children_begin + 2) == "z");
    std::cout << "PASS test_children_sorted\n";
}

// Verify case-insensitive sort.
static void test_children_sorted_insensitive() {
    FSTraits tr{CaseSensitivity::Insensitive, NormForm::None, 1, true};
    FSTreeBuilder b(tr);

    uint32_t root = b.add_entry(UINT32_MAX, "root", NODE_TYPE_DIR, 0, 0);
    b.add_entry(root, "Beta",  NODE_TYPE_FILE, 1, 0);
    b.add_entry(root, "alpha", NODE_TYPE_FILE, 2, 0);
    b.add_entry(root, "GAMMA", NODE_TYPE_FILE, 3, 0);

    CompactFSTree tree = b.build();
    auto ch = tree.children(0);
    assert(ch.size() == 3);
    // alpha < Beta < GAMMA  (case-insensitive)
    assert(tree.name(tree.node(0).children_begin + 0) == "alpha");
    assert(tree.name(tree.node(0).children_begin + 1) == "Beta");
    assert(tree.name(tree.node(0).children_begin + 2) == "GAMMA");
    std::cout << "PASS test_children_sorted_insensitive\n";
}

// Verify that build() leaves the builder empty and a second build() is safe.
static void test_builder_cleared_after_build() {
    FSTraits tr{CaseSensitivity::Sensitive, NormForm::None, 1, true};
    FSTreeBuilder b(tr);

    uint32_t root = b.add_entry(UINT32_MAX, "root", NODE_TYPE_DIR, 0, 0);
    b.add_entry(root, "child", NODE_TYPE_FILE, 1, 0);
    CompactFSTree t1 = b.build();
    assert(t1.size() == 2);

    // Rebuild from scratch.
    root = b.add_entry(UINT32_MAX, "r2", NODE_TYPE_DIR, 0, 0);
    CompactFSTree t2 = b.build();
    assert(t2.size() == 1);
    std::cout << "PASS test_builder_cleared_after_build\n";
}

// Verify import_subtree copies the subtree faithfully.
static void test_import_subtree() {
    FSTraits tr{CaseSensitivity::Sensitive, NormForm::None, 1, true};

    // Build source tree.
    FSTreeBuilder src(tr);
    uint32_t root = src.add_entry(UINT32_MAX, "root", NODE_TYPE_DIR, 0, 0);
    uint32_t a    = src.add_entry(root, "a",  NODE_TYPE_DIR,  0,   0);
    src.add_entry(a, "aa", NODE_TYPE_FILE, 100, 0);
    src.add_entry(a, "ab", NODE_TYPE_FILE, 200, 0);
    CompactFSTree source = src.build();

    // Build destination with import.
    FSTreeBuilder dst(tr);
    uint32_t new_root = dst.add_entry(UINT32_MAX, "root", NODE_TYPE_DIR, 0, 0);
    // Resolve 'a' in source (index of 'a' node).
    auto a_idx = source.resolve("a");
    assert(a_idx.has_value());
    dst.import_subtree(new_root, source, *a_idx);

    CompactFSTree result = dst.build();
    // root + imported(a, aa, ab) = 4 nodes
    assert(result.size() == 4);

    auto r = result.resolve("a/aa");
    assert(r.has_value());
    assert(result.node(*r).file_size == 100);
    std::cout << "PASS test_import_subtree\n";
}

// Empty tree should not crash.
static void test_empty_tree() {
    FSTraits tr{CaseSensitivity::Sensitive, NormForm::None, 1, true};
    FSTreeBuilder b(tr);
    CompactFSTree t = b.build();
    assert(t.size() == 0);
    assert(!t.resolve("anything").has_value());
    std::cout << "PASS test_empty_tree\n";
}

int main() {
    test_children_sorted();
    test_children_sorted_insensitive();
    test_builder_cleared_after_build();
    test_import_subtree();
    test_empty_tree();

    std::cout << "\nAll builder tests passed.\n";
    return 0;
}
