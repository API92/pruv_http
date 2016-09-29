/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/http_loop.hpp>

#include <cstring>

#include <pruv/log.hpp>

namespace pruv {
namespace http {

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

int http_loop::do_response_impl() noexcept
{
    try {
        if (strncmp(url_prefix, url(), url_prefix_len))
            return send_empty_response("404 Not Found");

        url_routing.go(url() + url_prefix_len, search_handler_result);
        if (search_handler_result.empty())
            return send_empty_response("404 Not Found");
        auto *hp = reinterpret_cast<decltype(handlers_holder)::value_type *>(
                search_handler_result.front());
        search_handler_result.clear();
        if (!hp->second)
            return send_empty_response("404 Not Found");

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
