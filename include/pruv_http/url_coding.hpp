/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <cstddef>
#include <forward_list>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pruv {
namespace http {

bool is_url_encoded(std::string_view url);
char * url_decode(char *url, size_t len);

class urlencoded_data :
    public std::unordered_map<std::string_view, std::string_view> {
public:
    urlencoded_data() = default;

    void parse(std::string_view query);

    urlencoded_data(urlencoded_data const &rhs);
    urlencoded_data & operator = (urlencoded_data const &rhs);

private:
    void copy_data(urlencoded_data const &rhs);

    std::forward_list<std::string> _holder;
};

} // namespace http
} // namespace pruv
