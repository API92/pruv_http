/*
 * Copyright (C) Andrey Pikas
 */

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <memory>

#include <dlfcn.h>
#include <getopt.h>
#include <glob.h>

#include <pruv/log.hpp>

#include <pruv_http/http_loop.hpp>

int parse_int_arg(const char *s, const char *optname)
{
    char *endptr = nullptr;
    unsigned long res = strtol(s, &endptr, 0);
    if ((res == ULONG_MAX && errno == ERANGE) || res > INT_MAX) {
        pruv_log(LOG_EMERG, "Options \"%s\" value \"%s\" is out of range",
                optname, s);
        exit(EXIT_FAILURE);
    }

    if (*endptr) {
        pruv_log(LOG_EMERG, "Options \"%s\" has non unsigned integer value",
                optname);
        exit(EXIT_FAILURE);
    }

    return (int)res;
}

struct dyn_lib_handle {
    void *handle;

    dyn_lib_handle(void *handle) : handle(handle) {}
    ~dyn_lib_handle()
    {
        if (!handle)
            return;
        dlerror();
        if (dlclose(handle)) {
            char const *error = dlerror();
            pruv_log(LOG_ERR, "dlclose error: %s.", error ? error : "");
        }
    }

    dyn_lib_handle(dyn_lib_handle &&rhs) : handle(rhs.handle)
    {
        rhs.handle = nullptr;
    }

    dyn_lib_handle(dyn_lib_handle const &) = delete;
    void operator = (dyn_lib_handle const &) = delete;
};

void load_handlers(char const *pattern, pruv::http::http_loop &loop,
        std::vector<dyn_lib_handle> &result)
{
    struct scoped_glob_t : glob_t {
        ~scoped_glob_t() {
            globfree(this);
        }
    };
    scoped_glob_t globbuf;

    if (int r = glob(pattern, 0, nullptr, &globbuf)) {
        char const *code = "Unknown code";
        if (r == GLOB_NOSPACE)
            code = "GLOB_NOSPACE";
        else if (r == GLOB_ABORTED)
            code = "GLOB_ABORTED";
        else if (r == GLOB_NOMATCH)
            code = "GLOB_NOMATCH";
        pruv_log(LOG_ERR, "Can't find handlers. glob returned %s.", code);
    }

    for (char **path = globbuf.gl_pathv; *path; ++path) {
        dyn_lib_handle h(dlopen(*path, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE));
        if (!h.handle) {
            pruv_log(LOG_ERR, "Can't load %s due to error %s.",
                    *path, dlerror());
            continue;
        }

        void (*registrator)(pruv::http::http_loop &) =
            reinterpret_cast<void (*)(pruv::http::http_loop &)>(dlsym(h.handle,
                    "_Z27register_pruv_http_handlersRN4pruv4http9http_loopE"));
        if (registrator) {
            pruv_log(LOG_INFO, "Founded http handlers registrator in %s.",
                    *path);
            registrator(loop);
            result.emplace_back(std::move(h));
        }
    }
}

int main(int argc, char * const *argv)
{
    int log_level = LOG_INFO;
    char const *handlers_pattern = "*.so";
    char const *url_prefix = "";
    const option opts[] = {
        {"loglevel", required_argument, &log_level, LOG_INFO},
        {"handlers-pattern", required_argument, NULL, 1},
        {"url-prefix", required_argument, NULL, 2},
        {0, 0, nullptr, 0}
    };

    for (;;) {
        int opt_idx = 0;
        int c = getopt_long(argc, argv, "", opts, &opt_idx);
        if (c == -1)
            break;
        if (c == 0) {
            if (optarg)
                *opts[opt_idx].flag = parse_int_arg(optarg, opts[opt_idx].name);
        }
        else if (c == 1)
            handlers_pattern = optarg;
        else if (c == 2)
            url_prefix = optarg;
        else if (c != '?') {
            pruv_log(LOG_EMERG, "Unknown option");
            return EXIT_FAILURE;
        }
    }
    pruv::openlog(pruv::log_type::JOURNALD, log_level);

    using pruv::http::http_loop;
    if (int r = http_loop::setup(argc, argv))
        return r;
    std::unique_ptr<http_loop> loop(new (std::nothrow) http_loop(url_prefix));
    if (!loop) {
        pruv_log(LOG_ERR, "No memory for worker loop object.");
        return EXIT_FAILURE;
    }

    std::vector<dyn_lib_handle> handles;
    load_handlers(handlers_pattern, *loop, handles);
    return loop->run();
}
