#pragma once

#include "core/compact_fs_tree.h"

#include <QString>

// Build a complete tree by recursively scanning from root_path.
CompactFSTree full_scan(const QString& root_path);

// Rebuild a tree, reusing unchanged subtrees from `existing`.
// Directories whose mtime has not changed (within FS resolution) are
// copied from `existing` without re-reading the filesystem.
CompactFSTree partial_rescan(const QString& root_path,
                              const CompactFSTree& existing);
