/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/url_fsm.hpp>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <utility>
#include <vector>

namespace pruv {
namespace http {

///
/// url_fsm::segment
///

struct url_fsm::segment {
    char const *r;
    char const *l;
    char *owned;

    segment() : r(""), l(r), owned(nullptr) {}
    segment(std::string const &s)
    {
        l = owned = new char[s.size()];
        memcpy(owned, s.c_str(), s.size());
        r = l + s.size();
    }

    segment(char const *l, char const *r)
    {
        auto dup_star_cmp = [](char a, char b) { return a == '*' && a == b; };
        if (std::adjacent_find(l, r, dup_star_cmp) == r) {
            this->l = l;
            this->r = r;
            this->owned = nullptr;
        }
        else {
            this->l = owned = new char[r - l];
            this->r = std::unique_copy(l, r, owned, dup_star_cmp);
        }
    }

    segment(char const *s) : segment(s, s + strlen(s)) {}

    void copy(segment const &rhs)
    {
        if (rhs.owned) {
            ptrdiff_t sz = rhs.r - rhs.l;
            l = owned = new char [sz];
            memcpy(owned, rhs.owned, sz);
            r = l + sz;
        }
        else {
            r = rhs.r;
            l = rhs.l;
            owned = nullptr;
        }
    }

    segment(segment const &rhs)
    {
        copy(rhs);
    }

    segment(segment &&rhs) : r(rhs.r), l(rhs.l), owned(rhs.owned)
    {
        rhs.owned = nullptr;
    }

    segment & operator = (segment const &rhs)
    {
        if (this != &rhs) {
            delete owned;
            owned = nullptr;
            copy(rhs);
        }
        return *this;
    }

    segment & operator = (segment &&rhs)
    {
        delete owned;
        owned = rhs.owned;
        rhs.owned = nullptr;
        r = rhs.r;
        l = rhs.l;
        return *this;
    }

    ~segment()
    {
        delete owned;
    }
};

struct url_fsm::node {
    node() : seg(), term(nullptr) {}
    node(segment seg) : seg(std::move(seg)), term(nullptr) {}

    segment seg; // Segment leading to this node.
    std::vector<std::pair<char, std::unique_ptr<node>>> next;
    std::unique_ptr<node> wildcard;
    void *term;
};


///
/// url_fsm::path
///


url_fsm::path::path() = default;
url_fsm::path::~path() = default;
url_fsm::path::path(path &&) = default;
url_fsm::path::path(path const &) = default;
url_fsm::path & url_fsm::path::operator = (path const &) = default;
url_fsm::path & url_fsm::path::operator = (path &&) = default;

url_fsm::path::path(char const *s) : _chain{s}
{
}

url_fsm::path & url_fsm::path::then(char const *s)
{
    _chain.emplace_back(s);
    return *this;
}

url_fsm::path & url_fsm::path::then(std::string s)
{
    _chain.emplace_back(std::move(s));
    return *this;
}

std::string_view
url_fsm::path::match(std::string_view p, std::vector<std::string_view> *w) const
{
    char const *s = p.data();
    char const *e = s + p.size();
    for (segment const &seg : _chain)
        for (char const *sl = seg.l, *sr = seg.r; sl != sr;)
            if (*sl == '*') {
                ++sl;
                char const *l = s;
                while (s != e && *s != '/')
                    ++s;
                if (w)
                    w->emplace_back(l, s - l);
                if (s == e)
                    return std::string_view(s, p.data() - s);
            }
            else {
                while (sl != sr && *sl != '*' && s != e && *sl == *s) {
                    ++sl;
                    ++s;
                }
                if (sl != sr && *sl != '*')
                    return std::string_view(s, p.data() - s);
            }

    return std::string_view(s, p.data() - s);
}


///
/// url_fsm
///

url_fsm::url_fsm() : _root(std::make_unique<node>())
{
}

url_fsm & url_fsm::operator = (url_fsm &&) = default;

url_fsm::~url_fsm()
{
}

url_fsm::node * url_fsm::split_segment(char const *s, std::unique_ptr<node> *vp)
{
    assert(vp);
    node *v = vp->get();
    if (s == v->seg.r)
        // Empty seg (for example, root or reached node) goes here.
        return v;
    assert(s);
    assert(s != v->seg.l); // Don't know parent.
    std::unique_ptr<node> mid(new node(segment(v->seg.l, s)));
    v->seg.l = s;
    if (*s == '*')
        mid->wildcard = std::move(*vp);
    else
        mid->next.emplace_back(*s, std::move(*vp));
    *vp = std::move(mid);
    return vp->get();
}

void url_fsm::add(path const &p, void *opaque)
{
    std::unique_ptr<node> *vp = &_root;
    node *v = vp->get();
    char const *pos = _root->seg.r;
    for (segment const &seg : p._chain)
        for (char const *s = seg.l; s != seg.r; ++s, ++pos)
            if (pos == v->seg.r || *pos != *s) {
                v = split_segment(pos, vp);
                if (*s == '*') {
                    if (!v->wildcard)
                        v->wildcard.reset(new node(segment(s, seg.r)));
                    vp = &v->wildcard;
                }
                else {
                    auto nit = std::lower_bound(v->next.begin(), v->next.end(),
                            std::make_pair(*s, std::unique_ptr<node>()));
                    if (nit == v->next.end() || nit->first != *s)
                        nit = v->next.emplace(nit,
                                *s, std::make_unique<node>(segment(s, seg.r)));
                    vp = &nit->second;
                }
                v = vp->get();
                pos = v->seg.l;
            }

    v = split_segment(pos, vp);
    v->term = opaque;
}

void url_fsm::go(std::string_view p, std::vector<void *> &result) const
{
    struct pos {
        node *v;
        char const *s;
        pos(node *v, char const *s) : v(v), s(s) {}
    };
    std::deque<pos> q(1, pos(_root.get(), _root->seg.r));
    std::deque<pos> qn;
    auto skip_first = [](node *v, int skip) { return pos(v, v->seg.l + skip); };
    for (; !p.empty(); p.remove_prefix(1), q.swap(qn)) {
        char c = p.front();
        while (!q.empty()) {
            pos st = q.front();
            q.pop_front();
            if (c == '/' && st.s != st.v->seg.r && *st.s == '*')
                ++st.s;
            if (st.s == st.v->seg.r) { // Node reached
                auto lb = std::lower_bound(st.v->next.begin(), st.v->next.end(),
                        std::make_pair(c, std::unique_ptr<node>()));
                if (lb != st.v->next.end() && lb->first == c)
                    qn.push_back(skip_first(lb->second.get(), 1));
                if (st.v->wildcard) {
                    if (c == '/')
                        q.push_back(skip_first(st.v->wildcard.get(), 1));
                    else
                        qn.push_back(skip_first(st.v->wildcard.get(), 0));
                }
            }
            else if (*st.s == '*' || c == *st.s++)
                qn.push_back(st);
        }
    }

    while (!q.empty()) {
        pos p = q.front();
        q.pop_front();
        if (p.s != p.v->seg.r && *p.s == '*')
            ++p.s;
        if (p.s == p.v->seg.r) {
            if (p.v->term)
                result.push_back(p.v->term);
            if (p.v->wildcard)
                q.push_front(skip_first(p.v->wildcard.get(), 0));
        }
    }
}

} // namespace http
} // namespace pruv
