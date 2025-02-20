/*
 * Copyright (c) 2020-2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <LibPartition/GUIDPartitionTable.h>

#ifndef KERNEL
#    include <LibCore/DeprecatedFile.h>
#endif

namespace Partition {

#define GPT_SIGNATURE2 0x54524150
#define GPT_SIGNATURE 0x20494645

struct [[gnu::packed]] GPTPartitionEntry {
    u8 partition_guid[16];
    u8 unique_guid[16];

    u64 first_lba;
    u64 last_lba;

    u64 attributes;
    char partition_name[72];
};

struct [[gnu::packed]] GUIDPartitionHeader {
    u32 sig[2];
    u32 revision;
    u32 header_size;
    u32 crc32_header;
    u32 reserved;
    u64 current_lba;
    u64 backup_lba;

    u64 first_usable_lba;
    u64 last_usable_lba;

    u64 disk_guid1[2];

    u64 partition_array_start_lba;

    u32 entries_count;
    u32 partition_entry_size;
    u32 crc32_entries_array;
};

#ifdef KERNEL
ErrorOr<NonnullOwnPtr<GUIDPartitionTable>> GUIDPartitionTable::try_to_initialize(Kernel::StorageDevice& device)
{
    auto table = TRY(adopt_nonnull_own_or_enomem(new (nothrow) GUIDPartitionTable(device)));
#else
ErrorOr<NonnullOwnPtr<GUIDPartitionTable>> GUIDPartitionTable::try_to_initialize(NonnullRefPtr<Core::DeprecatedFile> device_file)
{
    auto table = TRY(adopt_nonnull_own_or_enomem(new (nothrow) GUIDPartitionTable(move(device_file))));
#endif
    if (!table->is_valid())
        return Error::from_errno(EINVAL);
    return table;
}

#ifdef KERNEL
GUIDPartitionTable::GUIDPartitionTable(Kernel::StorageDevice& device)
    : MBRPartitionTable(device)
#else
GUIDPartitionTable::GUIDPartitionTable(NonnullRefPtr<Core::DeprecatedFile> device_file)
    : MBRPartitionTable(move(device_file))
#endif
{
    // FIXME: Handle OOM failure here.
    m_cached_header = ByteBuffer::create_zeroed(m_block_size).release_value_but_fixme_should_propagate_errors();
    VERIFY(partitions_count() == 0);
    if (!initialize())
        m_valid = false;
}

GUIDPartitionHeader const& GUIDPartitionTable::header() const
{
    return *(GUIDPartitionHeader const*)m_cached_header.data();
}

bool GUIDPartitionTable::initialize()
{
    VERIFY(m_cached_header.data() != nullptr);

    auto first_gpt_block = (m_block_size == 512) ? 1 : 0;

#ifdef KERNEL
    auto buffer = UserOrKernelBuffer::for_kernel_buffer(m_cached_header.data());
    if (!m_device->read_block(first_gpt_block, buffer))
        return false;
#else
    m_device_file->seek(first_gpt_block * m_block_size);
    if (m_device_file->read(m_cached_header.data(), m_cached_header.size()) != (int)m_block_size)
        return false;
#endif

    dbgln_if(GPT_DEBUG, "GUIDPartitionTable: signature - {:#08x} {:#08x}", header().sig[1], header().sig[0]);

    if (header().sig[0] != GPT_SIGNATURE && header().sig[1] != GPT_SIGNATURE2) {
        dbgln("GUIDPartitionTable: bad signature {:#08x} {:#08x}", header().sig[1], header().sig[0]);
        return false;
    }

    auto entries_buffer_result = ByteBuffer::create_zeroed(m_block_size);
    if (entries_buffer_result.is_error()) {
        dbgln("GUIDPartitionTable: not enough memory for entries buffer");
        return false;
    }
    auto entries_buffer = entries_buffer_result.release_value();
#ifdef KERNEL
    auto raw_entries_buffer = UserOrKernelBuffer::for_kernel_buffer(entries_buffer.data());
#endif
    size_t raw_byte_index = header().partition_array_start_lba * m_block_size;
    for (size_t entry_index = 0; entry_index < header().entries_count; entry_index++) {

#ifdef KERNEL
        if (!m_device->read_block((raw_byte_index / m_block_size), raw_entries_buffer))
            return false;
#else
        m_device_file->seek(raw_byte_index);
        if (m_device_file->read(entries_buffer.data(), entries_buffer.size()) != (int)m_block_size)
            return false;
#endif
        auto* entries = (GPTPartitionEntry const*)entries_buffer.data();
        auto& entry = entries[entry_index % (m_block_size / (size_t)header().partition_entry_size)];
        Array<u8, 16> partition_type {};
        partition_type.span().overwrite(0, entry.partition_guid, partition_type.size());

        if (is_unused_entry(partition_type)) {
            raw_byte_index += header().partition_entry_size;
            continue;
        }

        Array<u8, 16> unique_guid {};
        unique_guid.span().overwrite(0, entry.unique_guid, unique_guid.size());
        dbgln("Detected GPT partition (entry={}), offset={}, limit={}", entry_index, entry.first_lba, entry.last_lba);
        m_partitions.append({ entry.first_lba, entry.last_lba, partition_type, unique_guid, entry.attributes });
        raw_byte_index += header().partition_entry_size;
    }

    return true;
}

bool GUIDPartitionTable::is_unused_entry(Array<u8, 16> partition_type) const
{
    return all_of(partition_type, [](auto const octet) { return octet == 0; });
}

}
