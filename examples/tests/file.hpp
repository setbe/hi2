#include "../../hi/io.hpp"
#include "catch.hpp"

// These tests double as documentation.
// Key idea: prefer io::char_view / io::view<char> everywhere to avoid hidden strlen() and extra copies.

namespace test_file {

    // -------------------- Naming / Paths --------------------

    // Create a unique file name (no randomness, no time APIs).
    // We use an atomic counter so parallel test runs won't collide.
    static io::string unique_name(io::char_view base) noexcept {
        io::string s{};
        (void)s.append(base);
        (void)s.append("_");
        static io::atomic<io::u64> g{ 1 };
        io::u64 id = g.fetch_add(1, io::memory_order_relaxed);

        io::StackOut<64> ss{};
        ss << id << ".tmp";
        (void)s.append(ss.view());
        return s;
    }

    // Build "cwd + separator + name".
    // We intentionally do this in tests (instead of relying on relative paths),
    // because current working directory may differ between IDE and CI runners.
    static io::string make_path(io::char_view name) noexcept {
        io::string dir{};
        REQUIRE(fs::current_directory(dir));
        io::string p{};
        REQUIRE(p.append(dir));
        if (!p.empty()) {
            const char last = p[p.size()-1];
            if (last != fs::sep) {
                REQUIRE(p.push_back(fs::sep));
            }
        }
        REQUIRE(p.append(name));
        return p;
    }

    // Cleanup by FULL PATH (not by "name").
    //
    // Important: this must NOT abort the whole test from inside a helper.
    // If deletion fails we print a message so the failure is debuggable.
    //
    // On Windows, deletion may fail if some handle is still open.
    // (This is exactly what the RAII test is meant to detect.)
    static void cleanup_path(io::char_view path) noexcept {
        if (!path.data() || path.size() == 0) return;
        io::string p{};
        if (!p.append(path)) return;
        if (!fs::exists(p)) return;
        if (!fs::remove(p)) {
            io::out << "[cleanup] couldn't remove: " << p << '\n';
        }
    }

    // Convenience: open + read_all into string.
    // Using fs::File::read_all() ensures we exercise our own I/O stack.
    static io::string read_all_file(io::char_view path) {
        fs::File f(path, io::OpenMode::Read);
        REQUIRE(f.is_open());

        io::string out{};
        REQUIRE(f.read_all(out));
        return out;
    }

TEST_CASE("fs::File RAII: close on destruction (file can be reopened)", "[fs::File]") {
    // This test also guards against leaked Windows HANDLEs:
    // if the handle is leaked, rename/delete may fail later.
    io::string name = unique_name("io_file_test_raii");
    io::string path = make_path(name.as_view());

    cleanup_path(path.as_view());

    {
        fs::File f(path.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.good());
        REQUIRE(f.write_str("abc"));
    } // destructor must close the underlying OS handle

    REQUIRE(fs::exists(path));

    fs::File r(path.as_view(), io::OpenMode::Read);
    REQUIRE(r.is_open());

    io::string all{};
    REQUIRE(r.read_all(all));
    REQUIRE(all == "abc");

    cleanup_path(path.as_view());
}

TEST_CASE("fs::File write/read_all + size()", "[fs::File]") {
    const io::char_view name = "io_file_test_basic.txt";
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    {
        fs::File f(path.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());

        // All write APIs take io::char_view => no implicit strlen().
        REQUIRE(f.write_str("hello"));
        REQUIRE(f.write_str(" "));
        REQUIRE(f.write_str("world"));

        REQUIRE(f.flush());
        REQUIRE(f.good());

        // size() queries OS file size (not string length).
        REQUIRE(f.size() == 11);
    }

    io::string got = read_all_file(path.as_view());
    REQUIRE(got == "hello world");

    // Cross-check the platform filesystem layer too.
    REQUIRE(fs::file_size(path) == 11);

    cleanup_path(path.as_view());
}

TEST_CASE("fs::File truncate vs append", "[fs::File]") {
    const io::char_view name = "io_file_test_append.txt";
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    { // create + truncate
        fs::File f(path.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("A"));
    }
    REQUIRE(read_all_file(path.as_view()) == "A");

    { // append (must write at the end, not overwrite)
        fs::File f(path.as_view(), io::OpenMode::Append | io::OpenMode::Create);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("B"));
        REQUIRE(f.write_str("C"));
    }
    REQUIRE(read_all_file(path.as_view()) == "ABC");

    { // truncate again (must discard previous contents)
        fs::File f(path.as_view(), io::OpenMode::Write | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("Z"));
    }
    REQUIRE(read_all_file(path.as_view()) == "Z");

    cleanup_path(path.as_view());
}

TEST_CASE("fs::File seek/tell overwrite in middle", "[fs::File]") {
    const io::char_view name = "io_file_test_seek.txt";
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    {
        fs::File f(path.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());

        REQUIRE(f.write_str("0123456789"));
        REQUIRE(f.tell() == 10);

        // Seek to the middle and overwrite 2 bytes.
        REQUIRE(f.seek(5, io::SeekWhence::Begin));
        REQUIRE(f.tell() == 5);

        REQUIRE(f.write_str("XX"));
        REQUIRE(f.tell() == 7);

        REQUIRE(f.flush());
    }

    REQUIRE(read_all_file(path.as_view()) == "01234XX789");

    cleanup_path(path.as_view());
}

TEST_CASE("fs::File read_line handles LF and CRLF", "[fs::File]") {
    const io::char_view name = "io_file_test_lines.txt";
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    // Write a mix of LF and CRLF manually.
    // We do NOT rely on std::fstream or text mode conversions.
    {
        fs::File f(path.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());

        const char data[] = "one\r\ntwo\nthree\r\n";
        constexpr io::usize n = sizeof(data) - 1; // exclude '\0'
        REQUIRE(f.write(io::char_view{ data, n }) == n);
    }

    {
        fs::File r(path.as_view(), io::OpenMode::Read);
        REQUIRE(r.is_open());

        io::string line{};
        REQUIRE(r.read_line(line)); REQUIRE(line == "one");
        REQUIRE(r.read_line(line)); REQUIRE(line == "two");
        REQUIRE(r.read_line(line)); REQUIRE(line == "three");

        // After the last line, read_line() must return false and eof() must be true.
        REQUIRE_FALSE(r.read_line(line));
        REQUIRE(r.eof());
    }

    cleanup_path(path.as_view());
}

TEST_CASE("io::rename + fs::remove with fs::File", "[fs::File][fs]") {
    const io::char_view a = "io_file_test_rename_a.txt";
    const io::char_view b = "io_file_test_rename_b.txt";

    io::string pa = make_path(a);
    io::string pb = make_path(b);

    cleanup_path(pa.as_view());
    cleanup_path(pb.as_view());

    {
        fs::File f(pa.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("data"));
    }

    REQUIRE(fs::exists(pa));
    REQUIRE_FALSE(fs::exists(pb));

    REQUIRE(fs::rename(pa, pb));

    REQUIRE_FALSE(fs::exists(pa));
    REQUIRE(fs::exists(pb));

    REQUIRE(read_all_file(pb.as_view()) == "data");

    REQUIRE(fs::remove(pb));
    REQUIRE_FALSE(fs::exists(pb));

    cleanup_path(pa.as_view());
    cleanup_path(pb.as_view());
}

TEST_CASE("fs::File move semantics (move-construct + move-assign)", "[fs::File]") {
    const io::char_view name = "io_file_test_move.txt";
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    fs::File a(path.as_view(),
        io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
    REQUIRE(a.is_open());
    REQUIRE(a.write_str("hello"));

    fs::File b(static_cast<fs::File&&>(a)); // move-construct
    REQUIRE_FALSE(a.is_open());

    REQUIRE(b.is_open());
    REQUIRE(b.write_str(" world"));
    REQUIRE(b.flush());

    fs::File c;
    c = static_cast<fs::File&&>(b); // move-assign
    REQUIRE_FALSE(b.is_open());

    REQUIRE(c.is_open());
    c.close();

    REQUIRE(read_all_file(path.as_view()) == "hello world");

    cleanup_path(path.as_view());
}

TEST_CASE("fs::File supports UTF-8 paths (Cyrillic)", "[fs::File][unicode]") {
    // IO_U8("...") returns io::char_view, not const char*.
    // This means:
    // - no strlen()
    // - length is known at compile-time for string literals
    // - same code works in C++17 and C++20 (char vs char8_t literal)
    const io::char_view name = IO_U8("тест_файл.txt");
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    {
        fs::File f(path.as_view(),
            io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("hello"));
    }

    REQUIRE(fs::exists(path));
    REQUIRE(read_all_file(path.as_view()) == "hello");

    cleanup_path(path.as_view());
}

TEST_CASE("fs::File UTF-8 path with spaces and diacritics", "[fs::File][unicode]") {
    const io::char_view name = IO_U8("žluťoučký kůň.txt");
    io::string path = make_path(name);

    cleanup_path(path.as_view());

    {
        fs::File f(path.as_view(), io::OpenMode::Write | io::OpenMode::Create);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("data"));
    }

    REQUIRE(fs::file_size(path) == 4);

    cleanup_path(path.as_view());
}

TEST_CASE("directory_iterator works with UTF-8 names", "[fs][unicode]") {
    const io::char_view dir_name = IO_U8("тест_каталог");
    const io::char_view file_name = IO_U8("файл.txt");

    io::string dir_path = make_path(dir_name);

    // Best-effort cleanup. Directory removal might fail if it's not empty,
    // so we remove the file first later and then remove the directory.
    fs::remove(dir_path);
    bool created = fs::create_directory(dir_path);
    if (!created && !dir_path.empty())
        io::out << "Couldn't create dir: " << dir_path << '\n';
    REQUIRE(created);

    io::string file_path{};
    REQUIRE(fs::path_join(dir_path, file_name, file_path));

    // Ensure stale file is gone.
    cleanup_path(file_path.as_view());

    {
        fs::File f(file_path, io::OpenMode::Write | io::OpenMode::Create);
        REQUIRE(f.is_open());
        REQUIRE(f.write_str("x"));
    }

    bool found = false;
    for (fs::directory_iterator it{ dir_path.as_view() }; !it.is_end(); ++it) {
        // it->path is io::char_view.
        if (it->path.data() && it->path.find(file_name) != io::npos) {
            found = true;
            break;
        }
    }
    REQUIRE(found);

    // Cleanup: delete file then directory.
    cleanup_path(file_path.as_view());
    fs::remove(dir_path);
}

} // namespace
