#include "builder/fs_tree_builder.h"
#include "core/compact_fs_tree.h"
#include "core/node.h"
#include "serialization/fs_tree_serializer.h"

#include <cassert>
#include <iostream>
#include <string>

#include <QCoreApplication>
#include <QTemporaryFile>

static CompactFSTree make_tree() {
    FSTraits tr{CaseSensitivity::Insensitive, NormForm::None, 2'000'000'000ULL, false};
    FSTreeBuilder b(tr);
    uint32_t root  = b.add_entry(UINT32_MAX, "vol",  NODE_TYPE_DIR,  0,    10);
    uint32_t docs  = b.add_entry(root,  "docs",  NODE_TYPE_DIR,  0,    20);
    uint32_t hello = b.add_entry(docs,  "hello.txt", NODE_TYPE_FILE, 1234, 30);
    uint32_t pics  = b.add_entry(root,  "pics",  NODE_TYPE_DIR,  0,    40);
    uint32_t img   = b.add_entry(pics,  "photo.jpg", NODE_TYPE_FILE, 5678, 50);
    (void)hello; (void)img;
    return b.build();
}

static void test_roundtrip(const QString& tmp_path) {
    CompactFSTree original = make_tree();

    bool ok = FSTreeSerializer::save(original, tmp_path);
    assert(ok);

    auto loaded = FSTreeSerializer::load(tmp_path);
    assert(loaded.has_value());

    // Same node count.
    assert(loaded->size() == original.size());

    // FSTraits preserved.
    assert(loaded->traits().case_sensitivity   == original.traits().case_sensitivity);
    assert(loaded->traits().mtime_resolution_ns == original.traits().mtime_resolution_ns);
    assert(loaded->traits().has_file_ids        == original.traits().has_file_ids);

    // Resolve same paths.
    auto r1 = original.resolve("docs/hello.txt");
    auto r2 = loaded->resolve("docs/hello.txt");
    assert(r1.has_value() && r2.has_value());
    assert(original.node(*r1).file_size == loaded->node(*r2).file_size);
    assert(original.node(*r1).mtime     == loaded->node(*r2).mtime);

    // Case-insensitive resolve works after reload.
    auto r3 = loaded->resolve("DOCS/HELLO.TXT");
    assert(r3.has_value());
    assert(loaded->node(*r3).file_size == 1234);

    std::cout << "PASS test_roundtrip\n";
}

static void test_load_bad_magic(const QString& tmp_path) {
    // Write garbage.
    {
        QFile f(tmp_path);
        f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        f.write("NOTCFST!!", 9);
    }
    auto r = FSTreeSerializer::load(tmp_path);
    assert(!r.has_value());
    std::cout << "PASS test_load_bad_magic\n";
}

static void test_load_truncated(const QString& tmp_path) {
    CompactFSTree t = make_tree();
    FSTreeSerializer::save(t, tmp_path);
    // Truncate the file in the middle of nodes.
    {
        QFile f(tmp_path);
        f.open(QIODevice::WriteOnly | QIODevice::Append);
        // already saved; resize to header only
        f.resize(64 + 10); // 10 bytes into nodes — too short
    }
    auto r = FSTreeSerializer::load(tmp_path);
    assert(!r.has_value());
    std::cout << "PASS test_load_truncated\n";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QTemporaryFile tmp;
    tmp.open();
    tmp.close();
    QString path = tmp.fileName();

    test_roundtrip(path);
    test_load_bad_magic(path);
    test_load_truncated(path);

    std::cout << "\nAll serializer tests passed.\n";
    return 0;
}
