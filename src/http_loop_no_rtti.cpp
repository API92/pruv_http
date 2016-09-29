/*
 * Copyright (C) Andrey Pikas
 */

#include <pruv_http/http_loop.hpp>

#include <pruv/log.hpp>

namespace pruv {
namespace http {

int http_loop::do_response() noexcept
{
    return do_response_impl();
}

} // namespace http
} // namespace pruv
