/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pruv {
namespace http {

class url_fsm {
    struct segment;
public:
    class path {
    public:
        ~path();
        path();
        path(path const &);
        path(path &&);
        path & operator = (path const &);
        path & operator = (path &&);

        /// Star (*) character in pattern means "any until slash / or end".
        /// Pattern "*smth" not supported,
        /// only "foo*/" or "foo*/smth" or "foo*" supported
        path(char const *s);
        path(std::string s);
        path & then(char const *s);
        path & then(std::string s);

        std::string_view
        match(std::string_view p, std::vector<std::string_view> *w) const;

    private:
        std::vector<segment> _chain;
        friend url_fsm;
    };

    url_fsm();
    ~url_fsm();
    url_fsm & operator = (url_fsm &&);

    void add(path const &p, void *opaque);
    void go(std::string_view s, std::vector<void *> &res) const;

private:
    struct node;

    node * split_segment(char const *s, std::unique_ptr<node> *vp);

    std::unique_ptr<node> _root;
};

} // namespace http
} // namespace pruv
