/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/url_coding.hpp>

#include <cstdint>

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

} // namespace http
} // namespace pruv
