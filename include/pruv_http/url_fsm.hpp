/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace pruv {
namespace http {

class url_fsm {
public:
    enum segment_type : char {
        STRING,
        TILL_SLASH
    };

private:
    struct segment;

public:
    class path {
    public:
        ~path();
        path();
        path(path &&);
        path(path const &);
        path(char const *s);

        path & add(char const *s);
        path & add(segment_type t);


        char const * match(std::string_view p, std::vector<std::string_view> *w)
            const;

    private:
        template<typename T>
        inline path & add_impl(T &&arg);

        std::vector<segment> chain;

        friend url_fsm;
    };

    url_fsm();
    ~url_fsm();
    void add(path const &p, void *opaque);
    void go(std::string_view s, std::vector<void *> &res) const;

private:
    struct node;

    node * split_segment(char const *s, std::unique_ptr<node> *vp);

    std::unique_ptr<node> root;
};

} // namespace http
} // namespace pruv
