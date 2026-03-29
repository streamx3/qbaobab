#include "fs_tree_serializer.h"

#include "core/node.h"

#include <QFile>
#include <QtEndian>

#include <cstring>

// Helper: write a little-endian value of any integral type.
template<typename T>
static void write_le(QFile& f, T value) {
    // On little-endian platforms (x86/ARM LE) this is a no-op copy.
    // On big-endian platforms we byte-swap.
    static_assert(std::is_integral_v<T>);
    T le = value;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if constexpr (sizeof(T) == 2) le = qToLittleEndian(value);
    else if constexpr (sizeof(T) == 4) le = qToLittleEndian(value);
    else if constexpr (sizeof(T) == 8) le = qToLittleEndian(value);
#endif
    f.write(reinterpret_cast<const char*>(&le), sizeof(T));
}

template<typename T>
static T read_le(const char* buf) {
    T value;
    std::memcpy(&value, buf, sizeof(T));
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if constexpr (sizeof(T) == 2) value = qFromLittleEndian(value);
    else if constexpr (sizeof(T) == 4) value = qFromLittleEndian(value);
    else if constexpr (sizeof(T) == 8) value = qFromLittleEndian(value);
#endif
    return value;
}

// ---------------------------------------------------------------------------

bool FSTreeSerializer::save(const CompactFSTree& tree, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    const FSTraits& tr = tree.traits();
    uint64_t node_count  = tree.size();
    // Access internal vectors through the friend relationship.
    // We expose them via the serializer being a friend of CompactFSTree.
    const auto& nodes = tree.nodes_;
    const auto& names = tree.names_;
    uint64_t names_size  = static_cast<uint64_t>(names.size());

    // Header (64 bytes)
    write_le<uint32_t>(f, MAGIC);
    write_le<uint32_t>(f, VERSION);
    write_le<uint32_t>(f, static_cast<uint32_t>(sizeof(Node)));
    f.write(reinterpret_cast<const char*>(&tr.case_sensitivity), 1);
    f.write(reinterpret_cast<const char*>(&tr.name_normalization), 1);
    const char pad2[2] = {};
    f.write(pad2, 2);
    write_le<uint64_t>(f, tr.mtime_resolution_ns);
    uint8_t has_ids = tr.has_file_ids ? 1 : 0;
    f.write(reinterpret_cast<const char*>(&has_ids), 1);
    const char pad7[7] = {};
    f.write(pad7, 7);
    write_le<uint64_t>(f, node_count);
    write_le<uint64_t>(f, names_size);
    const char pad16[16] = {};
    f.write(pad16, 16);

    // Raw node data
    if (!nodes.empty()) {
        // On little-endian hosts the struct layout matches the file format directly.
        // On big-endian we'd need to byte-swap each field; not implemented here
        // since the spec's target platforms are all LE.
        f.write(reinterpret_cast<const char*>(nodes.data()),
                static_cast<qint64>(node_count * sizeof(Node)));
    }

    // String pool
    if (!names.empty()) {
        f.write(names.data(), static_cast<qint64>(names_size));
    }

    f.flush();
    return f.error() == QFileDevice::NoError;
}

std::optional<CompactFSTree> FSTreeSerializer::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;

    // Read header
    QByteArray hdr = f.read(HEADER_SIZE);
    if (hdr.size() < HEADER_SIZE)
        return std::nullopt;

    const char* p = hdr.constData();

    uint32_t magic   = read_le<uint32_t>(p);      p += 4;
    uint32_t version = read_le<uint32_t>(p);      p += 4;
    uint32_t node_sz = read_le<uint32_t>(p);      p += 4;

    if (magic   != MAGIC)           return std::nullopt;
    if (version != VERSION)         return std::nullopt;
    if (node_sz != sizeof(Node))    return std::nullopt;

    FSTraits tr{};
    tr.case_sensitivity   = static_cast<CaseSensitivity>(*reinterpret_cast<const uint8_t*>(p)); p += 1;
    tr.name_normalization = static_cast<NormForm>(*reinterpret_cast<const uint8_t*>(p));         p += 1;
    p += 2; // reserved

    tr.mtime_resolution_ns = read_le<uint64_t>(p); p += 8;
    tr.has_file_ids        = (*reinterpret_cast<const uint8_t*>(p)) != 0; p += 1;
    p += 7; // reserved

    uint64_t node_count = read_le<uint64_t>(p); p += 8;
    uint64_t names_size = read_le<uint64_t>(p); p += 8;
    // skip 16 reserved bytes
    // p += 16; (not needed; hdr buffer ends)

    // Sanity check against unreasonable sizes (> 4 GiB each).
    if (node_count > 0x1000'0000ULL || names_size > 0x4000'0000ULL)
        return std::nullopt;

    // Read nodes
    std::vector<Node> nodes;
    if (node_count > 0) {
        nodes.resize(static_cast<size_t>(node_count));
        qint64 bytes = static_cast<qint64>(node_count * sizeof(Node));
        if (f.read(reinterpret_cast<char*>(nodes.data()), bytes) != bytes)
            return std::nullopt;
    }

    // Read string pool
    std::vector<char> names_pool;
    if (names_size > 0) {
        names_pool.resize(static_cast<size_t>(names_size));
        qint64 bytes = static_cast<qint64>(names_size);
        if (f.read(names_pool.data(), bytes) != bytes)
            return std::nullopt;
    }

    CompactFSTree tree;
    tree.traits_ = tr;
    tree.nodes_  = std::move(nodes);
    tree.names_  = std::move(names_pool);
    return tree;
}
