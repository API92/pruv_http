/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/http_loop.hpp>

#include <pruv_http/status_codes.hpp>

void register_pruv_http_handlers(pruv::http::http_loop &loop)
{
    using pruv::http::http_loop;
    using pruv::http::url_fsm;
    loop.register_handler(url_fsm::path("/hello/world/"),
        [](http_loop &h) {
            h.start_response("HTTP/1.1", pruv::http::status_200);
            h.write_header("Content-Type", "html/text; charset=utf-8");
            if (!h.keep_alive())
                h.write_header("Connection", "close");
            h.complete_headers();
            char const body[] = u8"Hello world!!!\r\n";
            h.write_body(body, sizeof(body) - 1);
            h.complete_body();
            return 0;
        });

    loop.register_handler(url_fsm::path("/hi/mir/"),
        [](http_loop &h) {
            h.start_response("HTTP/1.1", pruv::http::status_200);
            h.write_header("Content-Type", "html/text; charset=utf-8");
            if (!h.keep_alive())
                h.write_header("Connection", "close");
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
            h.start_response("HTTP/1.1", pruv::http::status_200);
            h.write_header("Content-Type", "html/text; charset=utf-8");
            if (!h.keep_alive())
                h.write_header("Connection", "close");
            h.complete_headers();
            char const body[] = u8"Privet mir ";
            h.write_body(body, sizeof(body) - 1);
            h.write_body(args.at(0).first, args.at(0).second - args.at(0).first);
            h.write_body(u8"!!!\r\n", 5);
            h.complete_body();
            return 0;
        });
}
