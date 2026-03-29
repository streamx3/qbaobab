#include "scanner/scan_orchestrator.h"
#include "serialization/fs_tree_serializer.h"
#include "core/compact_fs_tree.h"
#include "core/node.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTextStream>

static QTextStream out(stdout);

static void print_usage(const char* argv0) {
    out << "Usage:\n"
        << "  " << argv0 << " scan  <path> [output.cfst]   -- full scan\n"
        << "  " << argv0 << " rescan <path> <input.cfst>    -- partial rescan\n"
        << "  " << argv0 << " info  <input.cfst>            -- print tree stats\n"
        << "  " << argv0 << " resolve <input.cfst> <rel/path> -- resolve path\n";
}

static void print_stats(const CompactFSTree& tree) {
    out << "Nodes:  " << tree.size() << "\n";
    out << "Traits: case="
        << (tree.traits().case_sensitivity == CaseSensitivity::Sensitive
            ? "sensitive" : "insensitive")
        << "  mtime_res=" << tree.traits().mtime_resolution_ns << " ns\n";

    uint32_t dirs = 0, files = 0, symlinks = 0;
    tree.visit_subtree(0, [&](uint32_t id) {
        uint16_t t = tree.node(id).flags & NODE_TYPE_MASK;
        if      (t == NODE_TYPE_DIR)     ++dirs;
        else if (t == NODE_TYPE_SYMLINK) ++symlinks;
        else if (t == NODE_TYPE_FILE)    ++files;
    });
    out << "Dirs:   " << dirs    << "\n";
    out << "Files:  " << files   << "\n";
    out << "Symlinks: " << symlinks << "\n";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const QString cmd   = argv[1];
    const QString arg1  = argv[2];
    const QString arg2  = (argc >= 4) ? argv[3] : QString{};

    if (cmd == "scan") {
        QElapsedTimer timer;
        timer.start();

        CompactFSTree tree = full_scan(arg1);

        qint64 elapsed = timer.elapsed();
        out << "Scanned " << tree.size() << " nodes in " << elapsed << " ms\n";
        print_stats(tree);

        if (!arg2.isEmpty()) {
            if (FSTreeSerializer::save(tree, arg2))
                out << "Saved to " << arg2 << "\n";
            else
                out << "ERROR: could not save to " << arg2 << "\n";
        }
        return 0;
    }

    if (cmd == "rescan") {
        if (arg2.isEmpty()) {
            print_usage(argv[0]);
            return 1;
        }
        auto existing = FSTreeSerializer::load(arg2);
        if (!existing) {
            out << "ERROR: could not load " << arg2 << "\n";
            return 1;
        }
        QElapsedTimer timer;
        timer.start();
        CompactFSTree tree = partial_rescan(arg1, *existing);
        qint64 elapsed = timer.elapsed();
        out << "Rescanned " << tree.size() << " nodes in " << elapsed << " ms\n";
        print_stats(tree);
        if (FSTreeSerializer::save(tree, arg2))
            out << "Updated " << arg2 << "\n";
        return 0;
    }

    if (cmd == "info") {
        auto tree = FSTreeSerializer::load(arg1);
        if (!tree) {
            out << "ERROR: could not load " << arg1 << "\n";
            return 1;
        }
        print_stats(*tree);
        return 0;
    }

    if (cmd == "resolve") {
        if (arg2.isEmpty()) {
            print_usage(argv[0]);
            return 1;
        }
        auto tree = FSTreeSerializer::load(arg1);
        if (!tree) {
            out << "ERROR: could not load " << arg1 << "\n";
            return 1;
        }
        auto idx = tree->resolve(arg2.toStdString());
        if (idx) {
            out << "Found node " << *idx << ": " << QString::fromStdString(tree->full_path(*idx)) << "\n";
        } else {
            out << "Not found: " << arg2 << "\n";
        }
        return 0;
    }

    print_usage(argv[0]);
    return 1;
}
