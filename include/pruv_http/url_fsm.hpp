/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <forward_list>
#include <memory>
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
        path(path &&other);
        path(char const *s);

        path & add(char const *s);
        path & add(segment_type t);


        char const * match(char const *s,
                std::vector<std::pair<char const *, char const *>> *w) const;

    private:
        template<typename T>
        inline path & add_impl(T &&arg);

        std::forward_list<segment> chain;
        std::forward_list<segment>::iterator last;

        friend url_fsm;
    };

    url_fsm();
    ~url_fsm();
    void add(path const &p, void *opaque);
    void go(char const *s, std::vector<void *> &result) const;

private:
    struct node;

    node * split_segment(char const *s, std::unique_ptr<node> *vp);

    std::unique_ptr<node> root;
};

} // namespace http
} // namespace pruv
