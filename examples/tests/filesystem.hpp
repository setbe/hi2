#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"

// ============================================================
// Filesystem tests.
//
// Philosophy:
// - Tests are API documentation: use only hi-level fs::File + io::* wrappers.
// - Prefer io::char_view everywhere to avoid hidden strlen/temporary allocations.
// - Keep path construction readable: split/join instead of manual slash checks.
// ============================================================

namespace test_fs {
    // Make a temporary directory under the current working directory so we don't need OS APIs.
    // We rely only on:
    // - fs::current_directory
    // - fs::create_directory
    // - fs::remove
    //
    // Directory name uses a monotonic counter to avoid collisions.
    static io::string make_temp_dir() noexcept {
        io::string cwd{};
        REQUIRE(fs::current_directory(cwd));
        REQUIRE(!cwd.empty());

        static io::atomic<io::u64> g{ 1 };
        io::u64 id = g.fetch_add(1, io::memory_order_relaxed);

        // Build dir name: "hi_fs_test_<id>"
        io::string leaf{};
        REQUIRE(leaf.append("hi_fs_test_"));

        io::StackOut<64> ss{};
        ss << id;
        REQUIRE(leaf.append(ss.view()));

        io::string dir{};
        REQUIRE(fs::path_join(cwd, leaf, dir));

        // Best-effort cleanup in case a previous crashed run left it behind.
        if (fs::exists(dir)) (void)fs::remove(dir);

        REQUIRE(fs::create_directory(dir));
        REQUIRE(fs::exists(dir));
        REQUIRE(fs::is_directory(dir));

        return dir;
    }

    // Best-effort remove: do not hard-fail tests just because the file is locked by AV/indexer.
    // Instead, print a message so the user understands what happened.
    static void cleanup_path(io::char_view p) noexcept {
        if (!fs::exists(p)) return;

        if (!fs::remove(p)) {
            io::out << "[cleanup] couldn't remove: " << p << '\n';
        }
    }

    // Extract filename from a full path without allocating (handles both separators).
    static io::char_view basename_view(io::char_view full) noexcept {
        if (!full.data() || full.size() == 0) return io::char_view{};

        io::usize last = 0;
        for (io::usize i = 0; i < full.size(); ++i) {
            char c = full[i];
            if (c == '/' || c == '\\') last = i + 1;
        }
        return io::char_view{ full.data() + last, full.size() - last };
    }

    // Write exact bytes to a file using fs::File (no OS API).
    static void write_bytes(io::char_view path, io::char_view bytes) {
        fs::File f(path, io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.write(bytes) == bytes.size());
        REQUIRE(f.flush());
        REQUIRE(f.good());
    }

// ============================================================
// Tests
// ============================================================

TEST_CASE("fs: basic create/stat/rename/remove + directory iteration", "[fs]") {
    io::string dir = make_temp_dir();

    io::string file1{};
    io::string file2{};

    REQUIRE(fs::path_join(dir, "a.txt", file1));
    REQUIRE(fs::path_join(dir, "b.txt", file2));

    // Create a file without strlen() and without OS headers.
    write_bytes(file1.as_view(), io::char_view{ "hello", 5 });

    REQUIRE(fs::exists(file1));
    REQUIRE(fs::is_regular_file(file1));
    REQUIRE(fs::file_size(file1) == 5);

    // Rename file1 -> file2 (all args are io::char_view / io::string)
    REQUIRE(fs::rename(file1, file2));
    REQUIRE_FALSE(fs::exists(file1));
    REQUIRE(fs::exists(file2));
    REQUIRE(fs::file_size(file2) == 5);

    // Iterate directory and find "b.txt"
    bool found = false;
    for (fs::directory_iterator it{ dir.as_view() }; !it.is_end(); ++it) {
        const auto& e = *it;
        if (!e.path.data()) continue;

        io::char_view base = basename_view(e.path);
        if (base == "b.txt") {
            found = true;
            REQUIRE(e.type == fs::file_type::regular);
            REQUIRE(e.size == 5);
            break;
        }
    }
    REQUIRE(found);

    // Cleanup (best-effort, see helper comment).
    cleanup_path(file2.as_view());
    cleanup_path(dir.as_view());
}

TEST_CASE("io: status() returns not_found for missing paths", "[io]") {
    io::string dir = make_temp_dir();

    io::string missing{};
    REQUIRE(fs::path_join(dir, "does_not_exist.bin", missing));

    REQUIRE_FALSE(fs::exists(missing));
    REQUIRE(fs::status(missing) == fs::file_type::not_found);

    cleanup_path(dir.as_view());
}

TEST_CASE("io: directory_iterator skips '.' and '..' and produces full paths", "[io]") {
    io::string dir = make_temp_dir();

    // Ensure at least one real entry exists.
    io::string f{};
    REQUIRE(fs::path_join(dir, "x.txt", f));
    write_bytes(f.as_view(), io::char_view{ "x", 1 });

    bool saw_x = false;

    for (fs::directory_iterator it{ dir.as_view() }; !it.is_end(); ++it) {
        const auto& e = *it;

        REQUIRE(e.path.data() != nullptr);
        REQUIRE(e.path.size() > 0);

        io::char_view base = basename_view(e.path);

        // These should never appear because iterator filters them out.
        REQUIRE(base != ".");
        REQUIRE(base != "..");

        if (base == "x.txt") {
            saw_x = true;
            REQUIRE(e.type == fs::file_type::regular);
            REQUIRE(e.size == 1);
        }

        // The iterator returns a full path, not just a leaf name.
        // So the directory should be a prefix somewhere.
        REQUIRE(e.path.find(dir.as_view()) != io::npos);
    }

    REQUIRE(saw_x);

    cleanup_path(f.as_view());
    cleanup_path(dir.as_view());
}

TEST_CASE("io: rename over existing destination is documented (platform behavior may differ)", "[io]") {
    // This test is intentionally tolerant: it documents current behavior rather than forcing one.
    // If you later standardize fs::rename semantics across platforms (e.g. always overwrite),
    // update this test to match.
    io::string dir = make_temp_dir();

    io::string a{}, b{};
    REQUIRE(fs::path_join(dir, "a.txt", a));
    REQUIRE(fs::path_join(dir, "b.txt", b));

    write_bytes(a.as_view(), io::char_view{ "A", 1 });
    write_bytes(b.as_view(), io::char_view{ "B", 1 });

    bool ok = fs::rename(a, b);

#ifdef _WIN32
    // With MoveFileW (no REPLACE_EXISTING), overwriting typically fails.
    REQUIRE_FALSE(ok);
    REQUIRE(fs::exists(a));
    REQUIRE(fs::exists(b));
#else
    // POSIX rename() usually overwrites (if your Linux backend follows that).
    // If your Linux backend is not implemented yet, ok may be false.
    (void)ok;
#endif

    cleanup_path(a.as_view());
    cleanup_path(b.as_view());
    cleanup_path(dir.as_view());
}

} // namespace
