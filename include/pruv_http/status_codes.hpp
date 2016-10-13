/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

namespace pruv {
namespace http {

extern char const *status_100; // Continue
extern char const *status_101; // Switching Protocols
extern char const *status_102; // Processing

extern char const *status_200; // OK
extern char const *status_201; // Created
extern char const *status_202; // Accepted
extern char const *status_203; // Non-Authoritative Information
extern char const *status_204; // No Content
extern char const *status_205; // Reset Content
extern char const *status_206; // Partial Content
extern char const *status_207; // Multi-Status
extern char const *status_208; // Already Reported
extern char const *status_226; // IM Used

extern char const *status_300; // Multiple Choices
extern char const *status_301; // Moved Permanently
extern char const *status_302; // Found
extern char const *status_303; // See Other
extern char const *status_304; // Not Modified
extern char const *status_305; // Use Proxy
extern char const *status_306; // Switch Proxy
extern char const *status_307; // Temporary Redirect
extern char const *status_308; // Permanent Redirect

extern char const *status_400; // Bad Request
extern char const *status_401; // Unauthorized
extern char const *status_402; // Payment Required
extern char const *status_403; // Forbidden
extern char const *status_404; // Not Found
extern char const *status_405; // Method Not Allowed
extern char const *status_406; // Not Acceptable
extern char const *status_407; // Proxy Authentication Required
extern char const *status_408; // Request Timeout
extern char const *status_409; // Conflict
extern char const *status_410; // Gone
extern char const *status_411; // Length Required
extern char const *status_412; // Precondition Failed
extern char const *status_413; // Payload Too Large
extern char const *status_414; // URI Too Long
extern char const *status_415; // Unsupported Media Type
extern char const *status_416; // Range Not Satisfiable
extern char const *status_417; // Expectation Failed
extern char const *status_418; // I'm a teapot
extern char const *status_421; // Misdirected Request
extern char const *status_422; // Unprocessable Entity
extern char const *status_423; // Locked
extern char const *status_424; // Failed Dependency
extern char const *status_426; // Upgrade Required
extern char const *status_428; // Precondition Required
extern char const *status_429; // Too Many Requests
extern char const *status_431; // Request Header Fields Too Large
extern char const *status_451; // Unavailable For Legal Reasons

extern char const *status_500; // Internal Server Error
extern char const *status_501; // Not Implemented
extern char const *status_502; // Bad Gateway
extern char const *status_503; // Service Unavailable
extern char const *status_504; // Gateway Timeout
extern char const *status_505; // HTTP Version Not Supported
extern char const *status_506; // Variant Also Negotiates
extern char const *status_507; // Insufficient Storage
extern char const *status_508; // Loop Detected
extern char const *status_510; // Not Extended
extern char const *status_511; // Network Authentication Required

} // namespace http
} //namespace pruv
