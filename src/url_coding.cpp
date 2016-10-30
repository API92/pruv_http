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

} // namespace

char * url_decode(char *s, size_t len)
{
    static int8_t const *hx = init_hex_val();
    char *out = s;
    char *ee = s + len;
    char const *e = ee - 2;
    int8_t h, l;
    while (s < e) {
        if (*s == '%' && (h = *(hx + s[1])) != -1 && (l = *(hx + s[2])) != -1) {
            *out++ = (h << 4) | l;
            s += 3;
            continue;
        }
        if (out != s)
            *out = *s;
        ++out;
        ++s;
    }

    if (out == s)
        return ee;

    while (s != ee)
        *out++ = *s++;

    return out;
}

} // namespace http
} // namespace pruv
