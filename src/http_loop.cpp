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
                    std::vector<std::string_view> &) {
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
            [f](http_loop &loop, std::vector<std::string_view> &) {
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
        if (http_parser_parse_url(url().data(), url().size(), is_connect, &u)
                != 0 || !(u.field_set & (1 << UF_PATH))) {
            respond_empty(status_400);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        _url_path = url().substr(
                u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);

        _url_query.clear();
        _url_query_holder.clear();
        if (u.field_set & (1 << UF_QUERY)) {
            std::string_view url_query = url().substr(
                    u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
            parse_url_query(url_query);
        }

        if (u.field_set & (1 << UF_FRAGMENT)) {
            _url_fragment = url().substr(
                u.field_data[UF_FRAGMENT].off, u.field_data[UF_FRAGMENT].len);
        }
        else
            _url_fragment = std::string_view();

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

void http_loop::parse_url_query(std::string_view query)
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

        if (is_url_encoded(key)) {
            _url_query_holder.emplace_front(key.data(), key.size());
            char *b = &_url_query_holder.front()[0];
            char *e = url_decode(b, key.size());
            key = std::string_view(b, e - b);
        }

        if (is_url_encoded(value)) {
            _url_query_holder.emplace_front(value.data(), value.size());
            char *b = &_url_query_holder.front()[0];
            char *e = url_decode(b, value.size());
            value = std::string_view(b, e - b);
        }

        _url_query[key] = value;
    }
}

std::string_view http_loop::cookie(char const *name) const
{
    std::string_view h(nullptr, 0);
    for (header const &ih : headers())
        if (strncasecmp(ih.field.data(), "Cookie", ih.field.size()) == 0) {
            h = ih.value;
            break;
        }

    if (!h.data())
        return h;

    auto trim = [](std::string_view &s) {
        s.remove_prefix(std::min(s.find_first_not_of(" \t"), s.size()));
        size_t trim_right = s.find_last_not_of(" \t");
        if (trim_right != std::string_view::npos)
            s.remove_suffix(s.size() - trim_right - 1);
    };

    while (!h.empty()) {
        std::string_view cname;
        size_t eq_pos = h.find_first_of('=');
        if (eq_pos != std::string_view::npos) {
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

    return std::string_view(nullptr, 0);
}

} // namespace http
} // namespace pruv
