/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <stdexcept>
#include <forward_list>
#include <functional>
#include <utility>

#include <pruv/http_worker.hpp>

#include <pruv_http/url_fsm.hpp>

namespace pruv {
namespace http {

class http_loop : public http_worker {
    typedef pruv::http_worker parent;

public:
    struct response_send_error : std::runtime_error {
        using runtime_error::runtime_error;
    };

    http_loop(char const *url_prefix);

    void start_response(char const *version, char const *status_line)
    {
        if (parent::start_response(version, status_line))
            return;
        throw response_send_error("start_response error");
    }

    void write_header(char const *name, char const *value)
    {
        if (parent::write_header(name, value))
            return;
        throw response_send_error("write_header error");
    }

    void complete_headers()
    {
        if (parent::complete_headers())
            return;
        throw response_send_error("complete_headers error");
    }

    void write_body(char const *data, size_t length)
    {
        if (parent::write_body(data, length))
            return;
        throw response_send_error("write_body error");
    }

    void complete_body()
    {
        if (parent::complete_body())
            return;
        throw response_send_error("complete_body error");
    }

    typedef std::function<int(http_loop &,
            std::vector<std::pair<char const *, char const *>> &)>
        args_handler_t;
    typedef std::function<int(http_loop &)> handler_t;

    void register_handler(url_fsm::path &&p, args_handler_t &&f);
    void register_handler(url_fsm::path &&p, handler_t &&f);

protected:
    virtual int do_response() noexcept override;

private:
    int do_response_impl() noexcept;

    char const *url_prefix;
    size_t url_prefix_len;
    url_fsm url_routing;
    std::forward_list<std::pair<url_fsm::path, args_handler_t>> handlers_holder;
    std::vector<void *> search_handler_result;
    std::vector<std::pair<char const *, char const *>> url_wildcards;
};

} // namespace http
} // namespace pruv
