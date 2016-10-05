/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/url_fsm.hpp>

#include <cassert>
#include <cstring>
#include <deque>
#include <set>
#include <utility>
#include <vector>

namespace pruv {
namespace http {

namespace {

inline bool isstop(char c)
{
    return !c || c == '?' || c == '#';
}

} // namespace


///
/// url_fsm::segment
///

struct url_fsm::segment {
    char const *r;
    char const *l;
    segment_type type;

    segment(char const *s) : r(s + strlen(s)), l(s), type(STRING) {}
    segment(char const *l, char const *r) : r(r), l(l), type(STRING) {}
    segment(segment_type type) : r(nullptr), l(nullptr), type(type) {}
};

struct url_fsm::node {
    node() : seg(""), term(nullptr) {}
    node(segment const &seg) : seg(seg), term(nullptr) {}

    segment seg; // Segment leading to this node.
    std::vector<std::pair<char, std::unique_ptr<node>>> next;
    std::unique_ptr<node> next_till_slash;
    void *term;
};


///
/// url_fsm::path
///


url_fsm::path::path() {}

url_fsm::path::~path() {}

url_fsm::path::path(path &&other) : chain(std::move(other.chain)) {}

url_fsm::path::path(path const &other) : chain(other.chain) {}

url_fsm::path::path(char const *s)
{
    chain.emplace_back(s);
}

template<typename T>
url_fsm::path & url_fsm::path::add_impl(T &&arg)
{
    chain.emplace_back(std::forward<T &&>(arg));
    return *this;
}

url_fsm::path & url_fsm::path::add(char const *s)
{
    return add_impl(s);
}

url_fsm::path & url_fsm::path::add(segment_type t)
{
    return add_impl(t);
}

char const * url_fsm::path::match(char const *s,
        std::vector<std::pair<char const *, char const *>> *w) const
{
    for (segment const &seg : chain)
        if (seg.type == STRING) {
            char const *m = seg.l;
            while (m != seg.r && !isstop(*s) && *m == *s) {
                ++m;
                ++s;
            }
            if (m != seg.r)
                return s;
        }
        else {
            char const *l = s;
            while (!isstop(*s) && *s != '/')
                ++s;
            if (w)
                w->emplace_back(l, s);
            if (*s != '/')
                return s;
        }

    return s;
}


///
/// url_fsm
///

url_fsm::url_fsm() : root(std::make_unique<node>())
{
}

url_fsm::~url_fsm()
{
}

url_fsm::node * url_fsm::split_segment(char const *s, std::unique_ptr<node> *vp)
{
    assert(vp);
    node *v = vp->get();
    if (s == v->seg.r)
        // Empty seg (for example, root or TILL_SLASH) goes here.
        return v;
    assert(v->seg.type == STRING);
    assert(s);
    assert(s != v->seg.l); // Don't know parent.
    std::unique_ptr<node> mid(new node(segment(v->seg.l, s)));
    v->seg.l = s;
    mid->next.emplace_back(*s, std::move(*vp));
    *vp = std::move(mid);
    return vp->get();
}

void url_fsm::add(path const &p, void *opaque)
{
    std::unique_ptr<node> *vp = &root;
    char const *pos = root->seg.r;
    for (segment const &seg : p.chain) {
        node *v = vp->get();
        if (seg.type == TILL_SLASH) {
            v = split_segment(pos, vp);
            if (!v->next_till_slash)
                v->next_till_slash.reset(new node(seg));
            vp = &v->next_till_slash;
            pos = (*vp)->seg.r;
            continue;
        }

        for (char const *s = seg.l; s != seg.r; ++s, ++pos)
            if (pos == v->seg.r || *pos != *s) {
                v = split_segment(pos, vp);
                auto nit = std::lower_bound(v->next.begin(), v->next.end(),
                        std::make_pair(*s, std::unique_ptr<node>()));
                bool new_node = (nit == v->next.end() || nit->first != *s);
                if (new_node)
                    nit = v->next.insert(nit, std::make_pair(*s,
                            std::make_unique<node>(segment(s, seg.r))));
                vp = &nit->second;
                v = vp->get();
                if (new_node) {
                    pos = v->seg.r;
                    break;
                }
                else
                    pos = v->seg.l;
            }
    }

    (*vp)->term = opaque;
}

void url_fsm::go(char const *s, std::vector<void *> &result) const
{
    struct pos {
        node *v;
        char const *s;
        pos(node *v, char const *s) : v(v), s(s) {}
        bool operator < (pos const &rhs) const {
            return s != rhs.s ? s < rhs.s : v < rhs.v;
        }
    };

    std::deque<pos> q(1, pos(root.get(), root->seg.r));
    std::set<pos> used;
    auto push_first = [&q, &used](node *v) {
        pos to(v, v->seg.l ? v->seg.l + 1 : nullptr);
        if (used.insert(to).second)
            q.push_back(to);
    };

    for (; !isstop(*s); ++s) {
        used.clear();
        for (size_t qsz = q.size(); qsz--;) {
            pos cur = q.front();
            q.pop_front();
            if (cur.v->seg.type == TILL_SLASH && *s != '/')
                push_first(cur.v);
            else {
                if (cur.s == cur.v->seg.r) { // Node reached
                    auto nit = std::lower_bound(
                            cur.v->next.begin(), cur.v->next.end(),
                            std::make_pair(*s, std::unique_ptr<node>()));
                    if (nit != cur.v->next.end() && nit->first == *s)
                        push_first(nit->second.get());
                    if (*s != '/' && cur.v->next_till_slash)
                        push_first(cur.v->next_till_slash.get());
                }
                else if (*s == *cur.s++ && used.insert(cur).second)
                    q.push_back(cur); // Next step on the path to the node
            }
        }
    }

    for (pos &p : q)
        if (p.s == p.v->seg.r && p.v->term)
            result.push_back(p.v->term);
}

} // namespace http
} // namespace pruv
