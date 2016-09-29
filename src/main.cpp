/*
 * Copyright (C) Andrey Pikas
 */

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <memory>

#include <getopt.h>

#include <pruv/log.hpp>

#include <pruv_http/http_loop.hpp>

int parse_int_arg(const char *s, const char *optname)
{
    char *endptr = nullptr;
    unsigned long res = strtol(s, &endptr, 0);
    if ((res == ULONG_MAX && errno == ERANGE) || res > INT_MAX) {
        pruv::log(LOG_EMERG, "Options \"%s\" value \"%s\" is out of range",
                optname, s);
        exit(EXIT_FAILURE);
    }

    if (*endptr) {
        pruv::log(LOG_EMERG, "Options \"%s\" has non unsigned integer value",
                optname);
        exit(EXIT_FAILURE);
    }

    return (int)res;
}

void register_handlers(pruv::http::http_loop &loop)
{
    using pruv::http::http_loop;
    using pruv::http::url_fsm;
    loop.register_handler(url_fsm::path("/hello/world/"),
        [](http_loop &h) {
            h.start_response("HTTP/1.1", "200 OK");
            h.write_header("Content-Type", "html/text; charset=utf-8");
            h.complete_headers();
            char const body[] = u8"Hello world!!!\r\n";
            h.write_body(body, sizeof(body) - 1);
            h.complete_body();
            return 0;
        });

    loop.register_handler(url_fsm::path("/hi/mir/"),
        [](http_loop &h) {
            h.start_response("HTTP/1.1", "200 OK");
            h.write_header("Content-Type", "html/text; charset=utf-8");
            h.complete_headers();
            char const body[] = u8"Hi mir!!!\r\n";
            h.write_body(body, sizeof(body) - 1);
            h.complete_body();
            return 0;
        });

    loop.register_handler(std::move(
        url_fsm::path("/privet/mir/").add(url_fsm::TILL_SLASH).add("/")),
        [](http_loop &h,
           std::vector<std::pair<char const *, char const *>> &args) {
            h.start_response("HTTP/1.1", "200 OK");
            h.write_header("Content-Type", "html/text; charset=utf-8");
            h.complete_headers();
            char const body[] = u8"Privet mir ";
            h.write_body(body, sizeof(body) - 1);
            h.write_body(args.at(0).first, args.at(0).second - args.at(0).first);
            h.write_body(u8"!!!\r\n", 5);
            h.complete_body();
            return 0;
        });
}

int main(int argc, char * const *argv)
{
    int log_level = LOG_INFO;
    const option opts[] = {
        {"loglevel", required_argument, &log_level, LOG_INFO},
        {0, 0, nullptr, 0}
    };

    for (;;) {
        int opt_idx = 0;
        int c = getopt_long(argc, argv, "", opts, &opt_idx);
        if (c == -1)
            break;
        if (c == 0 && optarg)
            *opts[opt_idx].flag = parse_int_arg(optarg, opts[opt_idx].name);
        else if (c != '?') {
            pruv::log(LOG_EMERG, "Unknown option");
            return EXIT_FAILURE;
        }
    }
    pruv::openlog(pruv::log_type::JOURNALD, log_level);

    using pruv::http::http_loop;
    if (int r = http_loop::setup())
        return r;
    std::unique_ptr<http_loop> loop(new (std::nothrow) http_loop("/prefix"));
    if (!loop) {
        pruv::log(LOG_ERR, "No memory for worker loop object.");
        return EXIT_FAILURE;
    }

    register_handlers(*loop);
    return loop->run();
}
