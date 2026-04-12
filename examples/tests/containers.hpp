#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"

// ---------- helper: lifetime tracker ----------
struct Tracker {
    static int alive;
    static int ctor;
    static int dtor;
    static int copy_ctor;
    static int move_ctor;
    static int copy_asg;
    static int move_asg;

    int v{ 0 };

    Tracker() noexcept { ++alive; ++ctor; }
    explicit Tracker(int x) noexcept : v(x) { ++alive; ++ctor; }

    Tracker(const Tracker& o) noexcept : v(o.v) { ++alive; ++copy_ctor; }
    Tracker(Tracker&& o) noexcept : v(o.v) { o.v = -777; ++alive; ++move_ctor; }

    Tracker& operator=(const Tracker& o) noexcept { v = o.v; ++copy_asg; return *this; }
    Tracker& operator=(Tracker&& o) noexcept { v = o.v; o.v = -888; ++move_asg; return *this; }

    ~Tracker() noexcept { --alive; ++dtor; }

    static void reset() {
        alive = ctor = dtor = copy_ctor = move_ctor = copy_asg = move_asg = 0;
    }
};
int Tracker::alive = 0;
int Tracker::ctor = 0;
int Tracker::dtor = 0;
int Tracker::copy_ctor = 0;
int Tracker::move_ctor = 0;
int Tracker::copy_asg = 0;
int Tracker::move_asg = 0;

// ============================================================
//                         vector tests
// ============================================================

TEST_CASE("io::vector default is empty", "[io][vector]") {
    io::vector<int> v;
    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() == 0);
    REQUIRE(v.empty());
    REQUIRE(v.data() == nullptr);
}

TEST_CASE("io::vector push_back lvalue/rvalue, front/back, operator[]", "[io][vector]") {
    io::vector<int> v;
    int a = 10;

    REQUIRE(v.push_back(a));
    REQUIRE(v.push_back(20));

    REQUIRE(v.size() == 2);
    REQUIRE(!v.empty());
    REQUIRE(v[0] == 10);
    REQUIRE(v[1] == 20);
    REQUIRE(v.front() == 10);
    REQUIRE(v.back() == 20);
}

TEST_CASE("io::vector reserve grows and preserves values", "[io][vector]") {
    io::vector<int> v;
    REQUIRE(v.reserve(1));
    REQUIRE(v.capacity() >= 1);
    REQUIRE(v.size() == 0);

    for (int i = 0; i < 50; ++i) REQUIRE(v.push_back(i));

    REQUIRE(v.size() == 50);
    for (int i = 0; i < 50; ++i) REQUIRE(v[(io::usize)i] == i);

    auto cap = v.capacity();
    REQUIRE(v.reserve(cap));          // no shrink
    REQUIRE(v.capacity() == cap);
}

TEST_CASE("io::vector resize grows value-initialized and shrinks destroying extras", "[io][vector]") {
    Tracker::reset();
    {
        io::vector<Tracker> v;

        REQUIRE(v.resize(5));
        REQUIRE(v.size() == 5);
        REQUIRE(Tracker::alive == 5);

        // grow more
        REQUIRE(v.resize(12));
        REQUIRE(v.size() == 12);
        REQUIRE(Tracker::alive == 12);

        // shrink
        REQUIRE(v.resize(3));
        REQUIRE(v.size() == 3);
        REQUIRE(Tracker::alive == 3);

        // clear destroys all
        v.clear();
        REQUIRE(v.size() == 0);
        REQUIRE(Tracker::alive == 0);
    }
    REQUIRE(Tracker::alive == 0);

    // exact count depends on moves during reserves; must be at least constructed amount
    REQUIRE(Tracker::dtor >= 12);
}

TEST_CASE("io::vector pop_back works and safe on empty", "[io][vector]") {
    Tracker::reset();
    io::vector<Tracker> v;

    v.pop_back(); // empty, must be safe
    REQUIRE(v.size() == 0);

    REQUIRE(v.push_back(Tracker{ 1 }));
    REQUIRE(v.push_back(Tracker{ 2 }));
    REQUIRE(v.size() == 2);
    REQUIRE(Tracker::alive == 2);

    v.pop_back();
    REQUIRE(v.size() == 1);
    REQUIRE(Tracker::alive == 1);

    v.pop_back();
    REQUIRE(v.size() == 0);
    REQUIRE(Tracker::alive == 0);

    v.pop_back();
    REQUIRE(v.size() == 0);
}

TEST_CASE("io::vector move ctor/assign transfers ownership and leaves source empty-ish", "[io][vector]") {
    io::vector<int> a;
    for (int i = 0; i < 10; ++i) REQUIRE(a.push_back(i));

    io::vector<int> b(io::move(a));
    REQUIRE(b.size() == 10);
    for (int i = 0; i < 10; ++i) REQUIRE(b[(io::usize)i] == i);

    // moved-from should be safe to destroy; size/cap aren't specified, but your code sets them to 0/null
    REQUIRE(a.size() == 0);
    REQUIRE(a.data() == nullptr);

    io::vector<int> c;
    REQUIRE(c.push_back(123));
    c = io::move(b);
    REQUIRE(c.size() == 10);
    REQUIRE(c[0] == 0);
    REQUIRE(c[9] == 9);

    REQUIRE(b.size() == 0);
    REQUIRE(b.data() == nullptr);
}

TEST_CASE("io::vector as_view points to same data and size", "[io][vector]") {
    io::vector<int> v;
    for (int i = 1; i <= 4; ++i) REQUIRE(v.push_back(i));

    auto vw = v.as_view();
    REQUIRE(vw.size() == 4);
    REQUIRE(vw.data() == v.data());
    REQUIRE(vw[0] == 1);
    REQUIRE(vw.back() == 4);

    const io::vector<int>& cv = v;
    auto cvw = cv.as_view();
    REQUIRE(cvw.size() == 4);
    REQUIRE(cvw.data() == v.data());
}


// ============================================================
//                       string tests
// ============================================================

TEST_CASE("io::string default is empty and nul-terminated", "[io][string]") {
    io::string s;
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.c_str()[0] == '\0');
}

TEST_CASE("io::string from cstr / append / push_back", "[io][string]") {
    io::string s("hi");
    REQUIRE(s.size() == 2);
    REQUIRE(s.c_str()[2] == '\0');

    REQUIRE(s.append(" there"));
    REQUIRE(s == io::view<const char>("hi there"));

    REQUIRE(s.push_back('!'));
    REQUIRE(s == io::view<const char>("hi there!"));
}

TEST_CASE("io::string resize grow/shrink", "[io][string]") {
    io::string s("ab");
    REQUIRE(s.resize(5, 'x'));
    REQUIRE(s == io::view<const char>("abxxx"));
    REQUIRE(s.c_str()[5] == '\0');

    REQUIRE(s.resize(1));
    REQUIRE(s == io::view<const char>("a"));
    REQUIRE(s.c_str()[1] == '\0');
}

TEST_CASE("io::wstring default is empty and nul-terminated", "[io][wstring]") {
    io::wstring ws;
    REQUIRE(ws.size() == 0);
    REQUIRE(ws.empty());
    REQUIRE(ws.c_str()[0] == L'\0');
}

TEST_CASE("io::wstring from wide cstr / append / push_back", "[io][wstring]") {
    io::wstring ws(L"ab");
    REQUIRE(ws.size() == 2);
    REQUIRE(ws.c_str()[2] == L'\0');

    REQUIRE(ws.append(L"cd"));
    REQUIRE(ws.size() == 4);
    REQUIRE(ws.c_str()[4] == L'\0');

    REQUIRE(ws.push_back(L'X'));
    REQUIRE(ws.size() == 5);
    REQUIRE(ws.c_str()[5] == L'\0');

    REQUIRE(ws[0] == L'a');
    REQUIRE(ws[3] == L'd');
    REQUIRE(ws[4] == L'X');
}

TEST_CASE("io::wstring resize grow/shrink", "[io][wstring]") {
    io::wstring ws(L"a");
    REQUIRE(ws.resize(3, L'z'));
    REQUIRE(ws.size() == 3);
    REQUIRE(ws[0] == L'a');
    REQUIRE(ws[1] == L'z');
    REQUIRE(ws[2] == L'z');
    REQUIRE(ws.c_str()[3] == L'\0');

    REQUIRE(ws.resize(0));
    REQUIRE(ws.size() == 0);
    REQUIRE(ws.c_str()[0] == L'\0');
}


// ============================================================
//                         deque tests
// ============================================================

TEST_CASE("io::deque default is empty", "[io][deque]") {
    io::deque<int> d;
    REQUIRE(d.size() == 0);
    REQUIRE(d.capacity() == 0);
    REQUIRE(d.empty());
}

TEST_CASE("io::deque push_back/pop_back basic", "[io][deque]") {
    io::deque<int> d;
    REQUIRE(d.push_back(1));
    REQUIRE(d.push_back(2));
    REQUIRE(d.push_back(3));

    REQUIRE(d.size() == 3);
    REQUIRE(d.front() == 1);
    REQUIRE(d.back() == 3);

    d.pop_back();
    REQUIRE(d.size() == 2);
    REQUIRE(d.back() == 2);

    d.pop_back();
    d.pop_back();
    REQUIRE(d.empty());

    d.pop_back(); // safe
    REQUIRE(d.size() == 0);
}

TEST_CASE("io::deque push_front/pop_front basic", "[io][deque]") {
    io::deque<int> d;
    REQUIRE(d.push_front(1));
    REQUIRE(d.push_front(2));
    REQUIRE(d.push_front(3)); // [3,2,1]

    REQUIRE(d.size() == 3);
    REQUIRE(d.front() == 3);
    REQUIRE(d.back() == 1);

    d.pop_front(); // [2,1]
    REQUIRE(d.size() == 2);
    REQUIRE(d.front() == 2);

    d.pop_front();
    d.pop_front();
    REQUIRE(d.empty());

    d.pop_front(); // safe
    REQUIRE(d.size() == 0);
}

TEST_CASE("io::deque wrap-around correctness (mixed pushes/pops)", "[io][deque]") {
    io::deque<int> d;

    // force capacity growth and front movement
    for (int i = 0; i < 20; ++i) REQUIRE(d.push_back(i));
    for (int i = 0; i < 10; ++i) d.pop_front(); // remove 0..9

    // now front is advanced; push more to wrap
    for (int i = 20; i < 40; ++i) REQUIRE(d.push_back(i));

    REQUIRE(d.size() == 30);
    // expected: 10..39
    for (int i = 0; i < 30; ++i) {
        REQUIRE(d[(io::usize)i] == (10 + i));
    }

    // push_front a few
    REQUIRE(d.push_front(9));
    REQUIRE(d.push_front(8));
    REQUIRE(d.front() == 8);
    REQUIRE(d[1] == 9);
    REQUIRE(d[2] == 10);
}

TEST_CASE("io::deque clear destroys elements", "[io][deque]") {
    Tracker::reset();
    io::deque<Tracker> d;
    for (int i = 0; i < 25; ++i) REQUIRE(d.push_back(Tracker{ i }));
    REQUIRE(Tracker::alive == 25);

    d.clear();
    REQUIRE(d.size() == 0);
    REQUIRE(Tracker::alive == 0);
}

// ============================================================
//                         list tests
// ============================================================

TEST_CASE("io::list default is empty", "[io][list]") {
    io::list<int> l;
    REQUIRE(l.size() == 0);
    REQUIRE(l.empty());
    REQUIRE(l.begin() == l.end());
}

TEST_CASE("io::list push_back/push_front and iteration order", "[io][list]") {
    io::list<int> l;
    REQUIRE(l.push_back(2));
    REQUIRE(l.push_front(1));
    REQUIRE(l.push_back(3)); // [1,2,3]

    REQUIRE(l.size() == 3);
    REQUIRE(l.front() == 1);
    REQUIRE(l.back() == 3);

    int expect = 1;
    for (auto it = l.begin(); it != l.end(); ++it, ++expect) {
        REQUIRE(*it == expect);
    }
}

TEST_CASE("io::list pop_front/pop_back and empty safety", "[io][list]") {
    io::list<int> l;
    l.pop_front();
    l.pop_back();
    REQUIRE(l.empty());

    REQUIRE(l.push_back(1));
    REQUIRE(l.push_back(2));
    REQUIRE(l.push_back(3));

    l.pop_front(); // [2,3]
    REQUIRE(l.front() == 2);
    REQUIRE(l.size() == 2);

    l.pop_back();  // [2]
    REQUIRE(l.back() == 2);
    REQUIRE(l.size() == 1);

    l.pop_back();  // []
    REQUIRE(l.empty());

    l.pop_back();
    l.pop_front();
    REQUIRE(l.size() == 0);
}

TEST_CASE("io::list erase middle/head/tail", "[io][list]") {
    io::list<int> l;
    for (int i = 1; i <= 5; ++i) REQUIRE(l.push_back(i)); // [1,2,3,4,5]

    // erase head (1)
    auto it = l.begin();
    it = l.erase(it);
    REQUIRE(l.size() == 4);
    REQUIRE(*it == 2);
    REQUIRE(l.front() == 2);

    // erase middle (3)
    ++it; // points to 3
    it = l.erase(it); // returns 4
    REQUIRE(l.size() == 3);
    REQUIRE(*it == 4);

    // erase tail (5)
    // move it to 5
    while (it != l.end()) ++it;
    // can't erase end; so find tail by iterating
    auto it2 = l.begin();
    while (true) {
        auto next = it2; ++next;
        if (next == l.end()) break;
        ++it2;
    }
    // it2 now points to last element
    REQUIRE(*it2 == 5);
    l.erase(it2);
    REQUIRE(l.size() == 2);
    REQUIRE(l.back() == 4);

    // remaining should be [2,4]
    auto a = l.begin();
    REQUIRE(*a == 2); ++a;
    REQUIRE(*a == 4); ++a;
    REQUIRE(a == l.end());
}

TEST_CASE("io::list clear destroys all nodes (Tracker)", "[io][list]") {
    Tracker::reset();
    {
        io::list<Tracker> l;
        for (int i = 0; i < 10; ++i) REQUIRE(l.push_back(Tracker{ i }));
        REQUIRE(Tracker::alive == 10);

        l.clear();
        REQUIRE(l.size() == 0);
        REQUIRE(Tracker::alive == 0);
    }
    REQUIRE(Tracker::alive == 0);
}

TEST_CASE("io::list move ctor/assign", "[io][list]") {
    io::list<int> a;
    for (int i = 0; i < 4; ++i) REQUIRE(a.push_back(i));

    io::list<int> b(io::move(a));
    REQUIRE(b.size() == 4);
    REQUIRE(a.size() == 0);

    int i = 0;
    for (auto x = b.begin(); x != b.end(); ++x, ++i) REQUIRE(*x == i);

    io::list<int> c;
    REQUIRE(c.push_back(99));
    c = io::move(b);
    REQUIRE(c.size() == 4);
    REQUIRE(b.size() == 0);
}