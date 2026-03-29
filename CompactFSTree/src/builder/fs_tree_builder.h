#pragma once

#include "core/compact_fs_tree.h"
#include "core/fs_traits.h"

#include <cstdint>
#include <string>
#include <vector>

struct TempNode {
    std::string           name;
    uint16_t              flags      = 0;
    uint64_t              file_size  = 0;
    uint64_t              mtime      = 0;
    uint32_t              parent     = UINT32_MAX; // index into temp_nodes_
    std::vector<uint32_t> children;               // indices into temp_nodes_
};

class FSTreeBuilder {
public:
    explicit FSTreeBuilder(FSTraits traits);

    // Add an entry. Returns its temp index. parent=UINT32_MAX for root.
    // Only one root (parent==UINT32_MAX) is allowed.
    uint32_t add_entry(uint32_t parent, std::string name,
                       uint16_t flags, uint64_t file_size, uint64_t mtime);

    // Bulk-import a subtree from an existing CompactFSTree (for partial rescan).
    // Recursively copies the subtree rooted at subtree_root as a child of new_parent.
    void import_subtree(uint32_t new_parent,
                        const CompactFSTree& source,
                        uint32_t subtree_root);

    // Consume the builder and produce a compact tree.
    // After this call the builder is reset to empty.
    CompactFSTree build();

private:
    FSTraits              traits_;
    std::vector<TempNode> temp_nodes_;

    // Recursive worker used by build().
    void build_recursive(uint32_t temp_id,
                         uint32_t my_final_index,
                         uint32_t parent_final,
                         uint32_t& counter,
                         std::vector<Node>& nodes,
                         std::vector<char>& pool) const;
};
