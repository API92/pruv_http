/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/url_coding.hpp>

#include <cstdint>
#include <unordered_set>

namespace pruv {
namespace http {

namespace {

static int8_t const * init_hex_val()
{
    static int8_t s[255];
    for (int i = 0; i <= 255; ++i)
        if ('0' <= i && i <= '9')
            s[i] = i - '0';
        else if ('A' <= i && i <= 'F')
            s[i] = i - 'A' + 10;
        else if ('a' <= i && i <= 'f')
            s[i] = i - 'a' + 10;
        else
            s[i] = -1;

    return s;
}

static int8_t const *hx = init_hex_val();

} // namespace

bool is_url_encoded(std::string_view url)
{
    if (url.size() < 3)
        return false;
    unsigned char const *s = reinterpret_cast<unsigned char const *>(url.data());
    unsigned char const *e = s + url.size() - 2;
    while (s < e) {
        if (*s == '%' && *(hx + s[1]) != -1 && *(hx + s[2]) != -1)
            return true;
        ++s;
    }

    return false;
}

char * url_decode(char *url, size_t len)
{
    char *out = url;
    unsigned char *s = (unsigned char *)url;
    unsigned char *ee = s + len;
    unsigned char const *e = ee - 2;
    int8_t h, l;
    while (s < e) {
        if (*s == '%' && (h = *(hx + s[1])) != -1 && (l = *(hx + s[2])) != -1) {
            *out++ = (h << 4) | l;
            s += 3;
            continue;
        }
        if (out != (void *)s)
            *out = *s;
        ++out;
        ++s;
    }

    if (out == (void *)s)
        return (char *)ee;

    while (s != ee)
        *out++ = *s++;

    return out;
}



void urlencoded_data::parse(std::string_view query)
{
    while (!query.empty()) {
        std::string_view key =
            query.substr(0, query.find_first_of("&="));
        query.remove_prefix(key.size());
        std::string_view value(nullptr, 0);
        if (!query.empty() && query.front() == '=') {
            query.remove_prefix(1);
            value = query.substr(0, query.find_first_of('&'));
            query.remove_prefix(value.size());
        }
        if (!query.empty())
            query.remove_prefix(1); // & after key or value

        if (is_url_encoded(key)) {
            _holder.emplace_front(key.data(), key.size());
            char *b = &_holder.front()[0];
            char *e = url_decode(b, key.size());
            key = std::string_view(b, e - b);
        }

        if (is_url_encoded(value)) {
            _holder.emplace_front(value.data(), value.size());
            char *b = &_holder.front()[0];
            char *e = url_decode(b, value.size());
            value = std::string_view(b, e - b);
        }

        (*this)[key] = value;
    }
}

void urlencoded_data::copy_data(urlencoded_data const &rhs)
{
    _holder = rhs._holder;
    std::unordered_set<std::string_view> s(_holder.begin(), _holder.end());
    for (auto const &p : rhs) {
        std::string_view key = p.first, value = p.second;
        auto it = s.find(key);
        if (it != s.end())
            key = *it;
        it = s.find(value);
        if (it != s.end())
            value = *it;
        (*this)[key] = value;
    }
}

urlencoded_data::urlencoded_data(urlencoded_data const &rhs)
{
    copy_data(rhs);
}

urlencoded_data & urlencoded_data::operator = (urlencoded_data const &rhs)
{
    if (this == &rhs)
        return *this;
    clear();
    copy_data(rhs);
    return *this;
}

} // namespace http
} // namespace pruv
