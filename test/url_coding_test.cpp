/*
 * Copyright (C) Andrey Pikas
 */

#include <gtest/gtest.h>

#include <pruv_http/url_coding.hpp>

namespace pruv::http {

TEST(urlencoded_data_parsing, test_1) {
    urlencoded_data d;
    std::unordered_map<std::string_view, std::string_view> ans;
    d.parse("key1=value1");
    ans = {{"key1", "value1"}};
    EXPECT_EQ(d, ans);

    d.parse("key2=value2");
    ans = {{"key1", "value1"}, {"key2", "value2"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key3=value3&key4=value4");
    ans = {{"key3", "value3"}, {"key4", "value4"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key1=value1&key2=value2&key3=value3");
    ans = {{"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key1=value1&key1=value2&key1=value3");
    ans = {{"key1", "value3"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("=value1&key2=value2");
    ans = {{"", "value1"}, {"key2", "value2"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key1=value1&=value2&key3=value3");
    ans = {{"key1", "value1"}, {"", "value2"}, {"key3", "value3"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key1=value1&key2=value2&=value3");
    ans = {{"key1", "value1"}, {"key2", "value2"}, {"", "value3"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("=value1");
    ans = {{"", "value1"}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("=");
    ans = {{"", ""}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("=&=&=");
    ans = {{"", ""}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key1=");
    ans = {{"key1", ""}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("key1=&key2=&key3=");
    ans = {{"key1", ""}, {"key2", ""}, {"key3", ""}};
    EXPECT_EQ(d, ans);

    d.clear();
    d.parse("%E2%A8%81=%E2%A8%82");
    ans = {{"⨁", "⨂"}};
    EXPECT_EQ(d, ans);
}

} // namespace pruv::http
