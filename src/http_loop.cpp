/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/http_loop.hpp>

#include <cstring>

#include <pruv/log.hpp>

#include <pruv_http/status_codes.hpp>

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

http_loop::http_loop(char const *url_prefix) :
    url_prefix(url_prefix), url_prefix_len(strlen(url_prefix)) {}

void http_loop::register_handler(url_fsm::path &&p, args_handler_t &&f)
{
    handlers_holder.emplace_front(std::move(p), std::move(f));
    url_routing.add(handlers_holder.front().first, &handlers_holder.front());
}

void http_loop::register_handler(url_fsm::path &&p, handler_t &&f)
{
    register_handler(std::move(p),
            [ff = std::move(f)](http_loop &loop,
                std::vector<std::pair<char const *, char const *>> &) {
                return ff(loop);
            });
}

void http_loop::register_handler(url_fsm::path const &p,
        args_handler_t const &f)
{
    handlers_holder.emplace_front(p, f);
    url_routing.add(handlers_holder.front().first, &handlers_holder.front());
}

void http_loop::register_handler(url_fsm::path const &p, handler_t const &f)
{
    register_handler(p,
            [f](http_loop &loop,
                std::vector<std::pair<char const *, char const *>> &) {
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
        if (strncmp(url_prefix, url(), url_prefix_len)) {
            respond_empty(status_404);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        url_routing.go(url() + url_prefix_len, search_handler_result);
        if (search_handler_result.empty()) {
            respond_empty(status_404);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        auto *hp = reinterpret_cast<decltype(handlers_holder)::value_type *>(
                search_handler_result.front());
        search_handler_result.clear();
        if (!hp->second) {
            respond_empty(status_404);
            return send_last_response() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        url_wildcards.clear();
        hp->first.match(url() + url_prefix_len, &url_wildcards);

        if (int ret = hp->second(*this, url_wildcards))
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

} // namespace http
} // namespace pruv
