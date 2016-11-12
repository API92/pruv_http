/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/http_loop.hpp>

#include <algorithm>
#include <cstring>

#include <strings.h>

#include <pruv/log.hpp>

#include <pruv_http/status_codes.hpp>
#include <pruv_http/url_coding.hpp>

namespace pruv {
namespace http {

http_loop::response_send_error::response_send_error(char const *s) :
    s(new char [strlen(s) + 1])
{
    strcpy(this->s, s);
}

http_loop::response_send_error::~response_send_error()
{
    delete[] s;
}

http_loop::response_send_error::response_send_error(
        response_send_error const &other)
{
    if (other.s)
        response_send_error(other.s);
    else
        s = nullptr;
}

http_loop::response_send_error::response_send_error(response_send_error &&other
        ) noexcept
    : s(other.s)
{
    other.s = nullptr;
}

http_loop::response_send_error & http_loop::response_send_error::operator = (
        response_send_error const &other)
{
    if (this == &other)
        return *this;

    delete[] s;
    s = nullptr;
    if (other.s) {
        s = new char [strlen(other.s) + 1];
        strcpy(s, other.s);
    }
    return *this;
}

http_loop::response_send_error & http_loop::response_send_error::operator = (
        response_send_error &&other) noexcept
{
    if (this == &other)
        return *this;

    delete[] s;
    s = other.s;
    other.s = nullptr;
    return *this;
}

void http_loop::register_handler(url_fsm::path &&p, args_handler_t &&f)
{
    _handlers_holder.emplace_front(std::move(p), std::move(f));
    _url_routing.add(_handlers_holder.front().first, &_handlers_holder.front());
}

void http_loop::register_handler(url_fsm::path &&p, handler_t &&f)
{
    register_handler(std::move(p),
            [ff = std::move(f)](http_loop &loop,
                    std::vector<std::experimental::string_view> &) {
                return ff(loop);
            });
}

void http_loop::register_handler(url_fsm::path const &p,
        args_handler_t const &f)
{
    _handlers_holder.emplace_front(p, f);
    _url_routing.add(_handlers_holder.front().first, &_handlers_holder.front());
}

void http_loop::register_handler(url_fsm::path const &p, handler_t const &f)
{
    register_handler(p,
            [f](http_loop &loop,
                    std::vector<std::experimental::string_view> &) {
                return f(loop);
            });
}

void http_loop::respond_empty(char const *status_line)
{
    start_response(u8"HTTP/1.1", status_line);
    if (!keep_alive())
        write_header(u8"Connection", u8"close");
    complete_headers();
    complete_body();
}

int http_loop::do_response_impl() noexcept
{
    try {
        http_parser_url u;
        http_parser_url_init(&u);
        int is_connect = method() == HTTP_CONNECT;
        if (http_parser_parse_url(url(), strlen(url()), is_connect, &u) != 0 ||
                !(u.field_set & (1 << UF_PATH))) {
            respond_empty(status_400);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        _url_path = url() + u.field_data[UF_PATH].off;
        url()[url() - _url_path + u.field_data[UF_PATH].len] = 0;

        _url_query.clear();
        if (u.field_set & (1 << UF_QUERY)) {
            char *url_query = url() + u.field_data[UF_QUERY].off;
            char *url_query_end = url_query + u.field_data[UF_QUERY].len;
            *url_query_end = 0;
            parse_url_query(url_query);
        }

        if (u.field_set & (1 << UF_FRAGMENT)) {
            _url_fragment = url() + u.field_data[UF_FRAGMENT].off;
            url()[url() - _url_fragment + u.field_data[UF_FRAGMENT].len] = 0;
        }
        else
            _url_fragment = "";

        _url_routing.go(url_path(), search_handler_result);
        if (search_handler_result.empty()) {
            respond_empty(status_404);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        auto *hp = reinterpret_cast<decltype(_handlers_holder)::value_type *>(
                search_handler_result.front());
        search_handler_result.clear();
        if (!hp->second) {
            respond_empty(status_404);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        _url_wildcards.clear();
        hp->first.match(url_path(), &_url_wildcards);

        if (int ret = hp->second(*this, _url_wildcards))
            return ret;

        if (!send_last_response())
            return EXIT_FAILURE;

        return EXIT_SUCCESS;
    }
    catch (response_send_error &e) {
        pruv_log(LOG_EMERG, "%s", e.what());
        return EXIT_FAILURE;
    }
    catch (std::exception &e) {
        pruv_log(LOG_EMERG, "%s", e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        pruv_log(LOG_EMERG, "Unknown error.");
        return EXIT_FAILURE;
    }
}

void http_loop::parse_url_query(char *query)
{
    while (*query) {
        char *key = query;
        size_t key_len = 0;
        char *value = nullptr;
        size_t value_len = 0;
        for (;;) {
            char c = *query;
            if (!c || c == '&') {
                if (value)
                    value_len = query - value;
                else
                    key_len = query - key;

                if (c)
                    ++query;
                break;
            }

            if (c == '=') {
                key_len = query - key;
                value = ++query;
                continue;
            }

            ++query;
        }

        if (key) {
            key_len = url_decode(key, key_len) - key;
            key[key_len] = 0;
        }

        if (value) {
            value_len = url_decode(value, value_len) - value;
            value[value_len] = 0;
        }

        _url_query[std::experimental::string_view(key, key_len)] =
                std::experimental::string_view(value, value_len);
    }
}

std::experimental::string_view http_loop::cookie(char const *name) const
{
    std::experimental::string_view h(nullptr, 0);
    for (header const &ih : headers())
        if (strcasecmp(ih.field, "Cookie") == 0) {
            h = ih.value;
            break;
        }

    if (!h.data())
        return h;

    auto trim = [](std::experimental::string_view &s) {
        s.remove_prefix(std::min(s.find_first_not_of(" \t"), s.size()));
        size_t trim_right = s.find_last_not_of(" \t");
        if (trim_right != std::experimental::string_view::npos)
            s.remove_suffix(s.size() - trim_right - 1);
    };

    while (!h.empty()) {
        std::experimental::string_view cname;
        size_t eq_pos = h.find_first_of('=');
        if (eq_pos != std::experimental::string_view::npos) {
            cname = h.substr(0, eq_pos);
            h.remove_prefix(cname.size() + 1);
        }
        auto cval = h.substr(0, h.find_first_of(';'));
        h.remove_prefix(std::min(cval.size() + 1, h.size()));

        trim(cname);
        if (strncasecmp(cname.data(), name, cname.size()) == 0) {
            trim(cval);
            return cval;
        }
    }

    return std::experimental::string_view(nullptr, 0);
}

} // namespace http
} // namespace pruv
