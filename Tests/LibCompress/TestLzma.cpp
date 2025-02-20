/*
 * Copyright (c) 2023, Tim Schumacher <timschumi@gmx.de>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibTest/TestCase.h>

#include <AK/MemoryStream.h>
#include <LibCompress/Lzma.h>

// The following tests are based on test files from the LZMA specification, which has been placed in the public domain.
// LZMA Specification Draft (2015): https://www.7-zip.org/a/lzma-specification.7z

Array<u8, 327> const specification_a_txt {
    0x4C, 0x5A, 0x4D, 0x41, 0x20, 0x64, 0x65, 0x63, 0x6F, 0x64, 0x65, 0x72, 0x20, 0x74, 0x65, 0x73,
    0x74, 0x20, 0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x0D, 0x0A, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x0D, 0x0A, 0x21, 0x20, 0x4C, 0x5A, 0x4D, 0x41, 0x20, 0x21, 0x20, 0x44,
    0x65, 0x63, 0x6F, 0x64, 0x65, 0x72, 0x20, 0x21, 0x20, 0x54, 0x45, 0x53, 0x54, 0x20, 0x21, 0x0D,
    0x0A, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x0D, 0x0A, 0x21, 0x20, 0x54, 0x45,
    0x53, 0x54, 0x20, 0x21, 0x20, 0x4C, 0x5A, 0x4D, 0x41, 0x20, 0x21, 0x20, 0x44, 0x65, 0x63, 0x6F,
    0x64, 0x65, 0x72, 0x20, 0x21, 0x0D, 0x0A, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x0D, 0x0A, 0x2D, 0x2D, 0x2D, 0x2D, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x4C, 0x69, 0x6E, 0x65,
    0x20, 0x31, 0x20, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x20, 0x0D, 0x0A, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x0D, 0x0A, 0x2D, 0x2D, 0x2D, 0x2D, 0x20, 0x54, 0x65,
    0x73, 0x74, 0x20, 0x4C, 0x69, 0x6E, 0x65, 0x20, 0x32, 0x20, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,
    0x2D, 0x2D, 0x20, 0x0D, 0x0A, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x0D, 0x0A,
    0x3D, 0x3D, 0x3D, 0x20, 0x45, 0x6E, 0x64, 0x20, 0x6F, 0x66, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20,
    0x66, 0x69, 0x6C, 0x65, 0x20, 0x3D, 0x3D, 0x3D, 0x3D, 0x20, 0x0D, 0x0A, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
    0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x0D, 0x0A
};

TEST_CASE(specification_a_lzma_decompress)
{
    Array<u8, 117> const compressed {
        0x5D, 0x00, 0x00, 0x80, 0x00, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x16,
        0x85, 0xBC, 0x45, 0xF0, 0xDF, 0xFF, 0xD2, 0xE8, 0x41, 0xF5, 0xCE, 0xE5, 0x90, 0xE1, 0xC8, 0x20,
        0xEA, 0xC6, 0x37, 0xBE, 0x2B, 0xD1, 0xF4, 0xC3, 0x34, 0x6F, 0x2F, 0x83, 0xC2, 0xA6, 0x7C, 0x6F,
        0x3D, 0x88, 0xA0, 0x58, 0x22, 0x1F, 0x3A, 0xBA, 0x7B, 0xC6, 0xDD, 0x66, 0xFE, 0xF8, 0x92, 0xE4,
        0xCB, 0x1C, 0xC4, 0x19, 0x0A, 0x0C, 0x8B, 0x2E, 0x39, 0xB8, 0xB8, 0x03, 0xCD, 0x5A, 0x9E, 0x10,
        0x3A, 0x4F, 0x65, 0xFA, 0x41, 0xCB, 0xF2, 0x79, 0x65, 0xD7, 0xF1, 0x9F, 0xAB, 0x70, 0x1D, 0x6F,
        0xF7, 0xB6, 0x79, 0xCC, 0x8A, 0x7D, 0xCE, 0xDB, 0xF8, 0xF6, 0x9E, 0xC9, 0x12, 0x9F, 0xAA, 0xBF,
        0x89, 0xFE, 0x05, 0x36, 0x80
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    auto buffer = MUST(decompressor->read_until_eof(PAGE_SIZE));
    EXPECT_EQ(specification_a_txt, buffer.span());
}

TEST_CASE(specification_a_eos_lzma_decompress)
{
    Array<u8, 122> const compressed {
        0x5D, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x26, 0x16,
        0x85, 0xBC, 0x45, 0xF0, 0xDF, 0xFF, 0xD2, 0xE8, 0x41, 0xF5, 0xCE, 0xE5, 0x90, 0xE1, 0xC8, 0x20,
        0xEA, 0xC6, 0x37, 0xBE, 0x2B, 0xD1, 0xF4, 0xC3, 0x34, 0x6F, 0x2F, 0x83, 0xC2, 0xA6, 0x7C, 0x6F,
        0x3D, 0x88, 0xA0, 0x58, 0x22, 0x1F, 0x3A, 0xBA, 0x7B, 0xC6, 0xDD, 0x66, 0xFE, 0xF8, 0x92, 0xE4,
        0xCB, 0x1C, 0xC4, 0x19, 0x0A, 0x0C, 0x8B, 0x2E, 0x39, 0xB8, 0xB8, 0x03, 0xCD, 0x5A, 0x9E, 0x10,
        0x3A, 0x4F, 0x65, 0xFA, 0x41, 0xCB, 0xF2, 0x79, 0x65, 0xD7, 0xF1, 0x9F, 0xAB, 0x70, 0x1D, 0x6F,
        0xF7, 0xB6, 0x79, 0xCC, 0x8A, 0x7D, 0xCE, 0xDB, 0xF8, 0xF6, 0x9E, 0xC9, 0x12, 0x9F, 0xAA, 0xBF,
        0x8A, 0x08, 0xF5, 0x99, 0x8D, 0x7F, 0xFA, 0x18, 0x0A, 0x52
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    auto buffer = MUST(decompressor->read_until_eof(PAGE_SIZE));
    EXPECT_EQ(specification_a_txt, buffer.span());
}

TEST_CASE(specification_a_eos_and_size_lzma_decompress)
{
    Array<u8, 122> const compressed {
        0x5D, 0x00, 0x00, 0x01, 0x00, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x16,
        0x85, 0xBC, 0x45, 0xF0, 0xDF, 0xFF, 0xD2, 0xE8, 0x41, 0xF5, 0xCE, 0xE5, 0x90, 0xE1, 0xC8, 0x20,
        0xEA, 0xC6, 0x37, 0xBE, 0x2B, 0xD1, 0xF4, 0xC3, 0x34, 0x6F, 0x2F, 0x83, 0xC2, 0xA6, 0x7C, 0x6F,
        0x3D, 0x88, 0xA0, 0x58, 0x22, 0x1F, 0x3A, 0xBA, 0x7B, 0xC6, 0xDD, 0x66, 0xFE, 0xF8, 0x92, 0xE4,
        0xCB, 0x1C, 0xC4, 0x19, 0x0A, 0x0C, 0x8B, 0x2E, 0x39, 0xB8, 0xB8, 0x03, 0xCD, 0x5A, 0x9E, 0x10,
        0x3A, 0x4F, 0x65, 0xFA, 0x41, 0xCB, 0xF2, 0x79, 0x65, 0xD7, 0xF1, 0x9F, 0xAB, 0x70, 0x1D, 0x6F,
        0xF7, 0xB6, 0x79, 0xCC, 0x8A, 0x7D, 0xCE, 0xDB, 0xF8, 0xF6, 0x9E, 0xC9, 0x12, 0x9F, 0xAA, 0xBF,
        0x8A, 0x08, 0xF5, 0x99, 0x8D, 0x7F, 0xFA, 0x18, 0x0A, 0x52
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    auto buffer = MUST(decompressor->read_until_eof(PAGE_SIZE));
    EXPECT_EQ(specification_a_txt, buffer.span());
}

TEST_CASE(specification_a_lp1_lc2_pb1_lzma_decompress)
{
    // Note: The name of this test file (and the accompanying info.txt) is wrong. It is encoded with lc = 1 instead of lc = 2.
    Array<u8, 117> const compressed {
        0x37, 0x00, 0x00, 0x01, 0x00, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x16,
        0x86, 0x23, 0xBC, 0x5C, 0xC9, 0x40, 0x2B, 0x6B, 0x91, 0x5B, 0xCD, 0x90, 0x40, 0xCB, 0x9A, 0x71,
        0x5B, 0x84, 0x68, 0xE0, 0x5A, 0xAB, 0xA3, 0xE9, 0x04, 0xF7, 0xA3, 0xA6, 0x8E, 0x5F, 0xAA, 0x24,
        0x8B, 0xFC, 0x20, 0x38, 0xA6, 0xB7, 0x2A, 0x47, 0xAF, 0x07, 0xF7, 0x14, 0xAC, 0xE8, 0xB4, 0xD9,
        0x96, 0x27, 0xE0, 0xF4, 0x47, 0x8D, 0xE9, 0xDD, 0x05, 0x28, 0x1A, 0xDF, 0xB1, 0xED, 0x1A, 0xDC,
        0x0B, 0x55, 0xB2, 0xBD, 0x55, 0x69, 0x6C, 0xD9, 0xFC, 0x70, 0x43, 0xA7, 0x16, 0x58, 0x99, 0xFE,
        0x97, 0x04, 0x11, 0x27, 0x56, 0x5E, 0xC6, 0xB0, 0x4E, 0x31, 0xA0, 0xCB, 0x17, 0x27, 0xEC, 0x72,
        0x36, 0x0E, 0x9A, 0xAD, 0x00
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    auto buffer = MUST(decompressor->read_until_eof(PAGE_SIZE));
    EXPECT_EQ(specification_a_txt, buffer.span());
}

TEST_CASE(specification_bad_corrupted_lzma_decompress)
{
    Array<u8, 117> const compressed {
        0x5D, 0x00, 0x00, 0x80, 0x00, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x16,
        0x85, 0xBC, 0x45, 0xF0, 0xDF, 0xFF, 0xD2, 0xE8, 0x41, 0xF5, 0xCE, 0xE5, 0x90, 0xE1, 0xC8, 0x20,
        0xEA, 0xC6, 0x37, 0xBE, 0x2B, 0xD1, 0xF4, 0xC3, 0x34, 0x6F, 0x2F, 0x83, 0xC2, 0xA6, 0x7C, 0x6F,
        0x3D, 0x88, 0xA0, 0x58, 0x22, 0x1F, 0x3A, 0xBA, 0x7B, 0xC6, 0xDD, 0x66, 0xFE, 0xF8, 0x92, 0xE4,
        0xCB, 0x1C, 0xC4, 0x19, 0x0A, 0x0C, 0x8B, 0x2E, 0x39, 0xB8, 0xB8, 0x03, 0xCD, 0x5A, 0x9E, 0x10,
        0x3A, 0x4F, 0x65, 0xFA, 0x41, 0xCB, 0xF2, 0x79, 0x65, 0xD7, 0xF1, 0xFF, 0xFF, 0xFF, 0x1D, 0x6F,
        0xF7, 0xB6, 0x79, 0xCC, 0x8A, 0x7D, 0xCE, 0xDB, 0xF8, 0xF6, 0x9E, 0xC9, 0x12, 0x9F, 0xAA, 0xBF,
        0x89, 0xFE, 0x05, 0x36, 0x80
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    auto buffer_or_error = decompressor->read_until_eof(PAGE_SIZE);
    EXPECT(buffer_or_error.is_error());
}

TEST_CASE(specification_bad_eos_incorrect_size_lzma_decompress)
{
    Array<u8, 122> const compressed {
        0x5D, 0x00, 0x00, 0x01, 0x00, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x16,
        0x85, 0xBC, 0x45, 0xF0, 0xDF, 0xFF, 0xD2, 0xE8, 0x41, 0xF5, 0xCE, 0xE5, 0x90, 0xE1, 0xC8, 0x20,
        0xEA, 0xC6, 0x37, 0xBE, 0x2B, 0xD1, 0xF4, 0xC3, 0x34, 0x6F, 0x2F, 0x83, 0xC2, 0xA6, 0x7C, 0x6F,
        0x3D, 0x88, 0xA0, 0x58, 0x22, 0x1F, 0x3A, 0xBA, 0x7B, 0xC6, 0xDD, 0x66, 0xFE, 0xF8, 0x92, 0xE4,
        0xCB, 0x1C, 0xC4, 0x19, 0x0A, 0x0C, 0x8B, 0x2E, 0x39, 0xB8, 0xB8, 0x03, 0xCD, 0x5A, 0x9E, 0x10,
        0x3A, 0x4F, 0x65, 0xFA, 0x41, 0xCB, 0xF2, 0x79, 0x65, 0xD7, 0xF1, 0x9F, 0xAB, 0x70, 0x1D, 0x6F,
        0xF7, 0xB6, 0x79, 0xCC, 0x8A, 0x7D, 0xCE, 0xDB, 0xF8, 0xF6, 0x9E, 0xC9, 0x12, 0x9F, 0xAA, 0xBF,
        0x8A, 0x08, 0xF5, 0x99, 0x8D, 0x7F, 0xFA, 0x18, 0x0A, 0x52
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    auto buffer_or_error = decompressor->read_until_eof(PAGE_SIZE);
    EXPECT(buffer_or_error.is_error());
}

TEST_CASE(specification_bad_incorrect_size_lzma_decompress)
{
    Array<u8, 117> const compressed {
        0x5D, 0x00, 0x00, 0x80, 0x00, 0x22, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x16,
        0x85, 0xBC, 0x45, 0xF0, 0xDF, 0xFF, 0xD2, 0xE8, 0x41, 0xF5, 0xCE, 0xE5, 0x90, 0xE1, 0xC8, 0x20,
        0xEA, 0xC6, 0x37, 0xBE, 0x2B, 0xD1, 0xF4, 0xC3, 0x34, 0x6F, 0x2F, 0x83, 0xC2, 0xA6, 0x7C, 0x6F,
        0x3D, 0x88, 0xA0, 0x58, 0x22, 0x1F, 0x3A, 0xBA, 0x7B, 0xC6, 0xDD, 0x66, 0xFE, 0xF8, 0x92, 0xE4,
        0xCB, 0x1C, 0xC4, 0x19, 0x0A, 0x0C, 0x8B, 0x2E, 0x39, 0xB8, 0xB8, 0x03, 0xCD, 0x5A, 0x9E, 0x10,
        0x3A, 0x4F, 0x65, 0xFA, 0x41, 0xCB, 0xF2, 0x79, 0x65, 0xD7, 0xF1, 0x9F, 0xAB, 0x70, 0x1D, 0x6F,
        0xF7, 0xB6, 0x79, 0xCC, 0x8A, 0x7D, 0xCE, 0xDB, 0xF8, 0xF6, 0x9E, 0xC9, 0x12, 0x9F, 0xAA, 0xBF,
        0x89, 0xFE, 0x05, 0x36, 0x80
    };

    auto stream = MUST(try_make<FixedMemoryStream>(compressed));
    auto decompressor = MUST(Compress::LzmaDecompressor::create_from_container(move(stream)));
    // FIXME: This should detect that there is still remaining (non-'end of stream') data after the expected data and reject the last read.
    //        As of now, this test accepts any result, except for crashes.
    (void)decompressor->read_until_eof(PAGE_SIZE);
}
