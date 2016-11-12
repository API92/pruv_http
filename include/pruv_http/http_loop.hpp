/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <experimental/string_view>
#include <forward_list>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <pruv/http_worker.hpp>

#include <pruv_http/url_fsm.hpp>

namespace pruv {
namespace http {

class http_loop : public http_worker {
    typedef pruv::http_worker parent;

public:
    class response_send_error : public std::exception {
    public:
        response_send_error(char const *s);
        ~response_send_error();
        response_send_error(response_send_error const &other);
        response_send_error(response_send_error &&other) noexcept;
        response_send_error & operator = (response_send_error const &other);
        response_send_error & operator = (response_send_error &&other) noexcept;

    private:
        char *s;
    };

    shmem_buffer & response_buf()
    {
        // Not null in handler because
        // send_last_response() called only after handler.
        return *get_response_buf();
    }

    char const * url_path() const { return _url_path; }
    char const * url_fragment() const { return _url_fragment; }
    auto & url_query() const { return _url_query; }

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

    void respond_empty(char const *status_line);

    void complete_body()
    {
        if (parent::complete_body())
            return;
        throw response_send_error("complete_body error");
    }

    std::experimental::string_view cookie(char const *name) const;

    typedef std::function<int(http_loop &,
            std::vector<std::experimental::string_view> &)>
        args_handler_t;
    typedef std::function<int(http_loop &)> handler_t;

    void register_handler(url_fsm::path &&p, args_handler_t &&f);
    void register_handler(url_fsm::path &&p, handler_t &&f);
    void register_handler(url_fsm::path const &p, args_handler_t const &f);
    void register_handler(url_fsm::path const &p, handler_t const &f);

protected:
    virtual int do_response() noexcept override;

private:
    int do_response_impl() noexcept;
    using parent::url;
    void parse_url_query(char *query);

    url_fsm _url_routing;
    char const *_url_path;
    char const *_url_fragment;
    std::unordered_map<
        std::experimental::string_view,
        std::experimental::string_view> _url_query;
    std::forward_list<std::pair<url_fsm::path, args_handler_t>>
            _handlers_holder;
    std::vector<void *> search_handler_result;
    std::vector<std::experimental::string_view> _url_wildcards;
};

} // namespace http
} // namespace pruv
