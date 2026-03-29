#pragma once

#include "core/compact_fs_tree.h"

#include <optional>
#include <QString>

// Binary serialisation of CompactFSTree.
//
// File layout (all multi-byte fields little-endian):
//   Offset  Size  Field
//     0       4   Magic "CFST"
//     4       4   Version (1)
//     8       4   sizeof(Node) — must be 40
//    12       1   CaseSensitivity
//    13       1   NormForm
//    14       2   Reserved
//    16       8   mtime_resolution_ns
//    24       1   has_file_ids
//    25       7   Reserved
//    32       8   node_count
//    40       8   names_size (bytes)
//    48      16   Reserved
//   ---     ---   ---
//    64   node_count * 40   raw Node data
//    64+N  names_size        raw string pool
class FSTreeSerializer {
public:
    // Write to file. Returns true on success.
    static bool save(const CompactFSTree& tree, const QString& path);

    // Read from file. Returns nullopt on any error.
    static std::optional<CompactFSTree> load(const QString& path);

private:
    static constexpr uint32_t MAGIC   = 0x54534643u; // "CFST" little-endian
    static constexpr uint32_t VERSION = 1u;
    static constexpr int      HEADER_SIZE = 64;
};
