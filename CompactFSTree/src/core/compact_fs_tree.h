#pragma once

#include "node.h"
#include "fs_traits.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class CompactFSTree {
public:
    CompactFSTree() = default;
    CompactFSTree(CompactFSTree&&) = default;
    CompactFSTree& operator=(CompactFSTree&&) = default;
    CompactFSTree(const CompactFSTree&) = delete;
    CompactFSTree& operator=(const CompactFSTree&) = delete;

    // --- Query API ---

    // Resolve a path to a node index. Segments separated by '/'.
    // Leading/trailing slashes are ignored. Empty path returns root (0).
    // Uses binary search per segment; comparison respects FSTraits::case_sensitivity.
    // Returns nullopt if any segment is not found or the tree is empty.
    std::optional<uint32_t> resolve(std::string_view path) const;

    // Direct children of node_id (contiguous in the nodes array, sorted by name).
    std::span<const Node> children(uint32_t node_id) const;

    // Zero-copy name view into the string pool.
    std::string_view name(uint32_t node_id) const;

    // Parent index (UINT32_MAX for root).
    uint32_t parent(uint32_t node_id) const;

    // Reconstruct full path by walking the parent chain (segments separated by '/').
    std::string full_path(uint32_t node_id) const;

    // Total node count.
    uint32_t size() const;

    // Access node by index.
    const Node& node(uint32_t node_id) const;

    // Access traits.
    const FSTraits& traits() const;

    // DFS preorder visitor.
    void visit_subtree(uint32_t root, std::function<void(uint32_t)> visitor) const;

private:
    friend class FSTreeBuilder;
    friend class FSTreeSerializer;

    FSTraits          traits_{};
    std::vector<Node> nodes_;  // all nodes; children of each node are contiguous
    std::vector<char> names_;  // UTF-8 NFC string pool

    // Compare two name strings according to traits_.
    // Returns negative/zero/positive (like strcmp).
    int compare_names(std::string_view a, std::string_view b) const;
};
