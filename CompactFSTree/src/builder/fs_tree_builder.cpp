#include "fs_tree_builder.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Name comparator helpers (mirrors compact_fs_tree logic)
// ---------------------------------------------------------------------------

static int ascii_tolower_cmp(const std::string& a, const std::string& b) {
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        int ca = std::tolower(static_cast<unsigned char>(a[i]));
        int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) return ca - cb;
    }
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return  1;
    return 0;
}

// ---------------------------------------------------------------------------
// FSTreeBuilder
// ---------------------------------------------------------------------------

FSTreeBuilder::FSTreeBuilder(FSTraits traits)
    : traits_(traits)
{}

uint32_t FSTreeBuilder::add_entry(uint32_t parent, std::string name_str,
                                   uint16_t flags, uint64_t file_size, uint64_t mtime) {
    uint32_t idx = static_cast<uint32_t>(temp_nodes_.size());
    TempNode tn;
    tn.name      = std::move(name_str);
    tn.flags     = flags;
    tn.file_size = file_size;
    tn.mtime     = mtime;
    tn.parent    = parent;
    temp_nodes_.push_back(std::move(tn));

    if (parent != UINT32_MAX) {
        assert(parent < temp_nodes_.size());
        temp_nodes_[parent].children.push_back(idx);
    }

    return idx;
}

void FSTreeBuilder::import_subtree(uint32_t new_parent,
                                    const CompactFSTree& source,
                                    uint32_t subtree_root) {
    const Node& src = source.node(subtree_root);
    std::string src_name(source.name(subtree_root));

    uint32_t new_id = add_entry(new_parent, std::move(src_name),
                                src.flags, src.file_size, src.mtime);

    for (uint32_t i = 0; i < src.children_count; ++i) {
        import_subtree(new_id, source, src.children_begin + i);
    }
}

// Pre-assigns consecutive final indices to the direct children of temp_id
// (before recursing), so that children are laid out contiguously in the
// output nodes_ vector.
void FSTreeBuilder::build_recursive(uint32_t temp_id,
                                     uint32_t my_final_index,
                                     uint32_t parent_final,
                                     uint32_t& counter,
                                     std::vector<Node>& nodes,
                                     std::vector<char>& pool) const {
    const TempNode& tn = temp_nodes_[temp_id];

    // Sort children by name using the FS comparator.
    // We sort a local copy of the children indices.
    std::vector<uint32_t> sorted_children = tn.children;
    if (traits_.case_sensitivity == CaseSensitivity::Insensitive) {
        std::sort(sorted_children.begin(), sorted_children.end(),
                  [&](uint32_t a, uint32_t b) {
                      return ascii_tolower_cmp(temp_nodes_[a].name,
                                               temp_nodes_[b].name) < 0;
                  });
    } else {
        std::sort(sorted_children.begin(), sorted_children.end(),
                  [&](uint32_t a, uint32_t b) {
                      return temp_nodes_[a].name < temp_nodes_[b].name;
                  });
    }

    // Pre-assign consecutive indices to direct children before recursing.
    uint32_t children_start = counter;
    counter += static_cast<uint32_t>(sorted_children.size());

    // Emit this node.
    Node& n      = nodes[my_final_index];
    n.name_offset = static_cast<uint32_t>(pool.size());
    n.name_len    = static_cast<uint16_t>(tn.name.size());
    n.flags       = tn.flags;
    n.parent      = parent_final;
    n.children_begin = children_start;
    n.children_count = static_cast<uint32_t>(sorted_children.size());
    n.file_size   = tn.file_size;
    n.mtime       = tn.mtime;
    pool.insert(pool.end(), tn.name.begin(), tn.name.end());

    // Recurse: each child occupies its pre-assigned index.
    for (uint32_t i = 0; i < static_cast<uint32_t>(sorted_children.size()); ++i) {
        build_recursive(sorted_children[i],
                        children_start + i,
                        my_final_index,
                        counter,
                        nodes,
                        pool);
    }
}

CompactFSTree FSTreeBuilder::build() {
    if (temp_nodes_.empty()) {
        temp_nodes_.clear();
        return CompactFSTree{};
    }

    // Find root (parent == UINT32_MAX).  Expect exactly one root at index 0.
    assert(temp_nodes_[0].parent == UINT32_MAX);

    const uint32_t total = static_cast<uint32_t>(temp_nodes_.size());

    std::vector<Node> nodes(total);
    std::vector<char> pool;
    pool.reserve(total * 16); // rough estimate

    // counter starts at 1 because root occupies index 0.
    uint32_t counter = 1;
    build_recursive(0, 0, UINT32_MAX, counter, nodes, pool);

    assert(counter == total);

    CompactFSTree tree;
    tree.traits_ = traits_;
    tree.nodes_  = std::move(nodes);
    tree.names_  = std::move(pool);

    // Clear builder state.
    temp_nodes_.clear();
    temp_nodes_.shrink_to_fit();

    return tree;
}
