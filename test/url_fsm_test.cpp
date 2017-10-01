/*
 * Copyright (C) Andrey Pikas
 */

#include <gtest/gtest.h>

#include <pruv_http/url_fsm.hpp>

namespace pruv::http {

namespace {

struct vec : std::vector<void *> {
    vec() = default;

    vec(std::initializer_list<int> l)
    {
        for (int v : l)
            push_back((void *)(intptr_t)v);
    }

    vec & operator = (std::initializer_list<int> l)
    {
        clear();
        for (int v : l)
            push_back((void *)(intptr_t)v);
        return *this;
    }
};

}

#define EXPECT_MATCHED(p, ...) \
    { std::vector<void *> res; vec ans = __VA_ARGS__; u.go(p, res); EXPECT_EQ(res, ans); }


TEST(url_fsm, test_1)
{
    url_fsm u;
    u.add("a", (void *)1);

    EXPECT_MATCHED("a", {1});
    EXPECT_MATCHED("b", {});
    EXPECT_MATCHED("", {});

    u.add("", (void *)2);

    EXPECT_MATCHED("", {2});

    u.add("abc", (void *)3);
    u.add("ac", (void *)4);

    EXPECT_MATCHED("a", {1});
    EXPECT_MATCHED("ab", {});
    EXPECT_MATCHED("abc", {3});
    EXPECT_MATCHED("abcd", {});
    EXPECT_MATCHED("ac", {4});
    EXPECT_MATCHED("acd", {});

    u = url_fsm();

    EXPECT_MATCHED("/abc/", {});
}

TEST(url_fsm, test_2)
{
    for (int i = 0; i < 2; ++i) {
        url_fsm u;

        std::pair<char const *, int> patterns[] = {
            {"/a/b/c/d",    1},
            {"/a/b/c/d/",   2},
            {"/a/b/c/*",    3},
            {"/a/b/c/**",   4},
            {"/a/b/c/***",  5},
            {"/a/b/c/****", 6},
            {"/a/b/c/*/",   7},
            {"/a/b/c/**/",  8},
            {"/a/b/*/d",    9},
            {"/a/b/**/d",   10},
            {"/a/*/c/d",    11},
            {"/*/b/c/d",    12},
            {"a/b",         13},
            {"*/b",         14},
            {"/a",          15},
            {"/ab",         16},
            {"/a*",         17},
            {"/ab/",        18},
            {"/a*/",        19},
        };
        if (i)
            std::reverse(patterns, std::end(patterns));

        for (auto const &p : patterns)
            u.add(p.first, (void *)(intptr_t)p.second);

        EXPECT_MATCHED("/a/b/c/d/f", {});
        EXPECT_MATCHED("/a/b/c/e", {(i ? 3 : 6)});
        EXPECT_MATCHED("/a/b/c/d", {1, (i ? 3 : 6), (i ? 9 : 10), 11, 12});
        EXPECT_MATCHED("/a/b/c/", {(i ? 3 : 6)});
        EXPECT_MATCHED("/a/b//d", {(i ? 9 : 10)});
        EXPECT_MATCHED("/a/b/d/", {});
        EXPECT_MATCHED("a/b", {13, 14});
        EXPECT_MATCHED("_/b", {14});
        EXPECT_MATCHED("/b", {14});
        EXPECT_MATCHED("/a", {15, 17});
        EXPECT_MATCHED("/ab", {16, 17});
        EXPECT_MATCHED("/abc", {17});
        EXPECT_MATCHED("/ac", {17});
        EXPECT_MATCHED("/a/", {19});
        EXPECT_MATCHED("/ab/", {18, 19});
        EXPECT_MATCHED("/abc/", {19});
        EXPECT_MATCHED("/ac/", {19});
    }
}

TEST(url_fsm, test_3)
{
    url_fsm u;
    u.add("/*a", nullptr);

    EXPECT_MATCHED("/a", {});
    EXPECT_MATCHED("/ba", {});
    EXPECT_MATCHED("//a", {});
}

TEST(url_fsm, test_4)
{
    url_fsm u;
    url_fsm::path const p("/prefix");
    u.add(url_fsm::path(p).then("*/foo"), (void *)1);
    u.add(url_fsm::path(p).then("infix*/foo"), (void *)2);
    u.add(url_fsm::path(p).then("/suffix"), (void *)3);

    EXPECT_MATCHED("/prefix/foo", {1});
    EXPECT_MATCHED("/prefix__/foo", {1});
    EXPECT_MATCHED("/prefixinfix/foo", {2, 1});
    EXPECT_MATCHED("/prefixinfix2/foo", {2, 1});
    EXPECT_MATCHED("/prefix/suffix", {3});
}

TEST(url_fsm, test_5)
{
    url_fsm u;
    u.add("/", (void *)1);
    u.add("/a", (void *)2);
    u.add("/ab", (void *)3);
    u.add("/abcde", (void *)4);
    u.add("/abcdefg", (void *)5);

    EXPECT_MATCHED("/", {1});
    EXPECT_MATCHED("/a", {2});
    EXPECT_MATCHED("/ab", {3});
    EXPECT_MATCHED("/abc", {});
    EXPECT_MATCHED("/abcd", {});
    EXPECT_MATCHED("/abcde", {4});
    EXPECT_MATCHED("/abcdef", {});
    EXPECT_MATCHED("/abcdefg", {5});
    EXPECT_MATCHED("/abcdefgh", {});
}

TEST(url_fsm, test_6)
{
    url_fsm u;
    u.add("/*", (void *)1);
    u.add("/a*", (void *)2);
    u.add("/ab*", (void *)3);
    u.add("/abcde*", (void *)4);
    u.add("/abcdefg*", (void *)5);

    EXPECT_MATCHED("/", {1});
    EXPECT_MATCHED("/a", {2, 1});
    EXPECT_MATCHED("/ab", {3, 2, 1});
    EXPECT_MATCHED("/abc", {3, 2, 1});
    EXPECT_MATCHED("/abcd", {3, 2, 1});
    EXPECT_MATCHED("/abcde", {4, 3, 2, 1});
    EXPECT_MATCHED("/abcdef", {4, 3, 2, 1});
    EXPECT_MATCHED("/abcdefg", {5, 4, 3, 2, 1});
    EXPECT_MATCHED("/abcdefgh", {5, 4, 3, 2, 1});
}

} // namespace pruv::http
