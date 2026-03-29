#include "scan_orchestrator.h"

#include "fs_scanner.h"
#include "builder/fs_tree_builder.h"
#include "core/node.h"

#include <QFileInfo>
#include <QString>

// ---------------------------------------------------------------------------
// Full scan
// ---------------------------------------------------------------------------

static void scan_recursive(FSScanner& scanner,
                            FSTreeBuilder& builder,
                            const QString& dir_path,
                            uint32_t parent_id) {
    auto entries = scanner.scan_directory(dir_path);

    for (auto& e : entries) {
        std::string norm_name = scanner.normalize_name(e.name);

        uint16_t flags = e.flags;
        uint32_t id = builder.add_entry(parent_id,
                                         std::move(norm_name),
                                         flags,
                                         e.file_size,
                                         e.mtime_ns);

        if ((flags & NODE_TYPE_MASK) == NODE_TYPE_DIR) {
            QString child_path = dir_path + '/' + QString::fromStdString(e.name);
            scan_recursive(scanner, builder, child_path, id);
        }
    }
}

CompactFSTree full_scan(const QString& root_path) {
    auto scanner = FSScanner::create(root_path);
    FSTreeBuilder builder(scanner->traits());

    // Add a root node representing the scanned directory itself.
    QFileInfo root_info(root_path);
    uint32_t root_id = builder.add_entry(
        UINT32_MAX,
        root_info.fileName().toStdString(),
        NODE_TYPE_DIR,
        0,
        0);

    scan_recursive(*scanner, builder, root_path, root_id);

    return builder.build();
}

// ---------------------------------------------------------------------------
// Partial rescan
// ---------------------------------------------------------------------------

// Walk existing tree in parallel with the filesystem.
// `existing_node` is the index in `existing` corresponding to dir_path.
static void rescan_recursive(FSScanner& scanner,
                              FSTreeBuilder& builder,
                              const QString& dir_path,
                              uint32_t parent_id,
                              const CompactFSTree& existing,
                              uint32_t existing_node) {
    const Node& enode = existing.node(existing_node);

    if (!scanner.has_changed(dir_path, enode.mtime)) {
        // Subtree is unchanged: bulk-import from existing tree.
        // Import each child of existing_node (not existing_node itself,
        // since the caller already created the directory node in builder).
        for (uint32_t i = 0; i < enode.children_count; ++i) {
            builder.import_subtree(parent_id, existing, enode.children_begin + i);
        }
        return;
    }

    // Directory has changed: rescan its entries.
    auto entries = scanner.scan_directory(dir_path);

    for (auto& e : entries) {
        std::string norm_name = scanner.normalize_name(e.name);

        uint16_t flags = e.flags;
        uint32_t new_id = builder.add_entry(parent_id,
                                             norm_name,
                                             flags,
                                             e.file_size,
                                             e.mtime_ns);

        if ((flags & NODE_TYPE_MASK) == NODE_TYPE_DIR) {
            QString child_path = dir_path + '/' + QString::fromStdString(e.name);

            // Try to find this child in the existing tree for recursive reuse.
            auto maybe_existing = existing.resolve(
                existing.full_path(existing_node) + '/' + norm_name);

            if (maybe_existing.has_value()) {
                rescan_recursive(scanner, builder, child_path, new_id,
                                 existing, *maybe_existing);
            } else {
                // New directory not present in existing tree: full scan.
                scan_recursive(scanner, builder, child_path, new_id);
            }
        }
    }
}

CompactFSTree partial_rescan(const QString& root_path,
                              const CompactFSTree& existing) {
    auto scanner = FSScanner::create(root_path);
    FSTreeBuilder builder(scanner->traits());

    QFileInfo root_info(root_path);
    uint32_t root_id = builder.add_entry(
        UINT32_MAX,
        root_info.fileName().toStdString(),
        NODE_TYPE_DIR,
        0,
        0);

    rescan_recursive(*scanner, builder, root_path, root_id, existing, 0);

    return builder.build();
}
