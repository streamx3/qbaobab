#include "compact_fs_tree.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <vector>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static int ascii_tolower_cmp(std::string_view a, std::string_view b) {
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
// CompactFSTree
// ---------------------------------------------------------------------------

int CompactFSTree::compare_names(std::string_view a, std::string_view b) const {
    if (traits_.case_sensitivity == CaseSensitivity::Insensitive)
        return ascii_tolower_cmp(a, b);
    // Case-sensitive: plain byte-wise comparison
    int r = a.compare(b);
    // Normalise to -1/0/1 for consistency (not required, but tidy).
    return (r < 0) ? -1 : (r > 0) ? 1 : 0;
}

std::optional<uint32_t> CompactFSTree::resolve(std::string_view path) const {
    if (nodes_.empty()) return std::nullopt;

    // Strip leading/trailing slashes
    while (!path.empty() && path.front() == '/') path.remove_prefix(1);
    while (!path.empty() && path.back()  == '/') path.remove_suffix(1);

    if (path.empty()) return 0u; // root

    uint32_t current = 0;

    while (!path.empty()) {
        auto slash = path.find('/');
        std::string_view segment;
        if (slash == std::string_view::npos) {
            segment = path;
            path    = {};
        } else {
            segment = path.substr(0, slash);
            path    = path.substr(slash + 1);
        }
        if (segment.empty()) continue; // skip double //

        const Node& n = nodes_[current];

        // Binary search within the contiguous children block.
        uint32_t lo = n.children_begin;
        uint32_t hi = n.children_begin + n.children_count;

        bool found = false;
        while (lo < hi) {
            uint32_t mid     = lo + (hi - lo) / 2;
            std::string_view mid_name = name(mid);
            int cmp = compare_names(segment, mid_name);
            if (cmp == 0) {
                current = mid;
                found   = true;
                break;
            } else if (cmp < 0) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }

        if (!found) return std::nullopt;
    }

    return current;
}

std::span<const Node> CompactFSTree::children(uint32_t node_id) const {
    assert(node_id < nodes_.size());
    const Node& n = nodes_[node_id];
    if (n.children_count == 0) return {};
    return std::span<const Node>(nodes_.data() + n.children_begin, n.children_count);
}

std::string_view CompactFSTree::name(uint32_t node_id) const {
    assert(node_id < nodes_.size());
    const Node& n = nodes_[node_id];
    return std::string_view(names_.data() + n.name_offset, n.name_len);
}

uint32_t CompactFSTree::parent(uint32_t node_id) const {
    assert(node_id < nodes_.size());
    return nodes_[node_id].parent;
}

std::string CompactFSTree::full_path(uint32_t node_id) const {
    std::vector<std::string_view> parts;
    uint32_t cur = node_id;
    while (cur != UINT32_MAX && cur < nodes_.size()) {
        std::string_view n = name(cur);
        if (!n.empty()) parts.push_back(n);
        cur = nodes_[cur].parent;
    }
    std::reverse(parts.begin(), parts.end());

    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += '/';
        result += parts[i];
    }
    return result;
}

uint32_t CompactFSTree::size() const {
    return static_cast<uint32_t>(nodes_.size());
}

const Node& CompactFSTree::node(uint32_t node_id) const {
    assert(node_id < nodes_.size());
    return nodes_[node_id];
}

const FSTraits& CompactFSTree::traits() const {
    return traits_;
}

void CompactFSTree::visit_subtree(uint32_t root,
                                   std::function<void(uint32_t)> visitor) const {
    if (root >= nodes_.size()) return;
    visitor(root);
    const Node& n = nodes_[root];
    for (uint32_t i = 0; i < n.children_count; ++i) {
        visit_subtree(n.children_begin + i, visitor);
    }
}
