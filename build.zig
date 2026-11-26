// Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//! The no-OS build of this libc (YASLIBC_NOOS): a static library for code with
//! no kernel beneath it, the YasOS kernel first of all. The userspace build is
//! the Makefile; this one only ever produces the freestanding variant.
//!
//! What changes from userspace is the bottom layer. noos/syscalls.c stands in
//! for syscalls.c with newlib-style porting hooks (_read, _write, _sbrk, _exit,
//! ...) that the embedding program overrides, and noos/malloc.c for the mmap
//! allocator with one grown through _sbrk. Everything above that -- string,
//! stdio, stdlib, ctype, scanf -- is the same source userspace runs.
//!
//! Exposes the static library as the artifact "yaslibc" and its headers as the
//! named lazy path "include".

const std = @import("std");

/// The portable part of libc: the same files userspace builds, picked for
/// needing nothing from the OS beyond what noos/syscalls.c provides.
const portable_sources = [_][]const u8{
    // atoi.c is yasc too (strto*'s `out char * borrows(s) * view` end
    // pointer), and the kernel calls none of it: the image dropped every
    // function in it at --gc-sections (stdlib.c's atoll, which calls
    // strtoll, with them).
    "bitops.c",
    // byteswap.c, ctype.c and fcntl.c are yasc as well (Makefile: YASC_SRCS;
    // cast<...>), which clang cannot build.  <byteswap.h> and <ctype.h> give
    // their callers inline versions, and nothing without an OS opens a file.
    "errno.c",
    "fnmatch.c",
    // libgen.c is yasc (slices: `#pragma yasc`), which clang cannot build;
    // nothing without an OS calls dirname/basename (Makefile: YASC_SRCS).
    "localtime.c",
    "qsort.c",
    "rand.c",
    "scanf.c",
    "stdio.c",
    "stdlib.c",
    "strftime.c",
    "string.c",
    "stringc.c",
    "strings.c",
    "wchar.c",
    "wctype.c",
};

const noos_sources = [_][]const u8{
    "noos/malloc.c",
    "noos/syscalls.c",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "yaslibc",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = false,
        }),
    });

    const flags = [_][]const u8{
        "-std=gnu11",
        "-DYASLIBC_NOOS",
        // libc's own sources are never fortified (sys/_fortify.h; the Makefile
        // undefines it too): zig's clang defines _FORTIFY_SOURCE when it
        // optimizes, and <string.h>'s macros would rewrite strcpy's own
        // definition into a __builtin___strcpy_chk call.
        "-U_FORTIFY_SOURCE",
        // The string routines are the loops a compiler would otherwise turn
        // back into calls to themselves.
        "-fno-builtin",
        "-ffunction-sections",
        "-fdata-sections",
    };

    lib.root_module.addCSourceFiles(.{ .files = &portable_sources, .flags = &flags });
    lib.root_module.addCSourceFiles(.{ .files = &noos_sources, .flags = &flags });
    if (target.result.cpu.arch.isArm() or target.result.cpu.arch.isThumb()) {
        lib.root_module.addCSourceFiles(.{ .files = &.{"noos/aeabi_mem.c"}, .flags = &flags });
    }
    lib.root_module.addIncludePath(b.path("include"));
    // Like newlib, which this replaces: libc is not where a sanitizer's traps
    // belong -- the word-wise copies type-pun on purpose.
    lib.root_module.sanitize_c = .off;

    b.installArtifact(lib);
    b.addNamedLazyPath("include", b.path("include"));
}
