#pragma once

#include <cstdint>

// Bit layout of Node::flags
static constexpr uint16_t NODE_TYPE_MASK    = 0x0003;
static constexpr uint16_t NODE_TYPE_FILE    = 0x0000;
static constexpr uint16_t NODE_TYPE_DIR     = 0x0001;
static constexpr uint16_t NODE_TYPE_SYMLINK = 0x0002;
static constexpr uint16_t NODE_TYPE_OTHER   = 0x0003;
static constexpr uint16_t NODE_FLAG_HIDDEN  = 0x0004;

struct Node {
    uint32_t name_offset;      // byte offset into string pool
    uint16_t name_len;         // name length in bytes
    uint16_t flags;            // NODE_TYPE_* | NODE_FLAG_*
    uint32_t parent;           // index into nodes vector (UINT32_MAX for root)
    uint32_t children_begin;   // index of first child in nodes vector
    uint32_t children_count;   // number of direct children
    uint64_t file_size;        // bytes; 0 for directories
    uint64_t mtime;            // nanoseconds since Unix epoch
};

// Natural alignment produces 4 bytes padding before file_size, giving 40 bytes total.
static_assert(sizeof(Node) == 40, "Node must be exactly 40 bytes");
