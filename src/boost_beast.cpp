/*
This file is largely copied from boost/beast/src.hpp
Unfortuately the test streams don't compile with -fno-rtti at the moment, so
those files are commented out
*/

#define BOOST_BEAST_SOURCE

#include <boost/beast/core/detail/config.hpp>

#include <boost/beast/_experimental/test/impl/error.ipp>
#include <boost/beast/_experimental/test/impl/fail_count.ipp>
//#include <boost/beast/_experimental/test/impl/stream.ipp>
//#include <boost/beast/_experimental/test/detail/stream_state.ipp>

#include <boost/beast/core/detail/base64.ipp>
#include <boost/beast/core/detail/impl/temporary_buffer.ipp>
#include <boost/beast/core/detail/sha1.ipp>
#include <boost/beast/core/impl/error.ipp>
#include <boost/beast/core/impl/file_posix.ipp>
#include <boost/beast/core/impl/file_stdio.ipp>
#include <boost/beast/core/impl/file_win32.ipp>
#include <boost/beast/core/impl/flat_static_buffer.ipp>
#include <boost/beast/core/impl/saved_handler.ipp>
#include <boost/beast/core/impl/static_buffer.ipp>
#include <boost/beast/core/impl/string.ipp>
#include <boost/beast/http/detail/basic_parser.ipp>
#include <boost/beast/http/detail/rfc7230.ipp>
#include <boost/beast/http/impl/basic_parser.ipp>
#include <boost/beast/http/impl/error.ipp>
#include <boost/beast/http/impl/field.ipp>
#include <boost/beast/http/impl/fields.ipp>
#include <boost/beast/http/impl/rfc7230.ipp>
#include <boost/beast/http/impl/status.ipp>
#include <boost/beast/http/impl/verb.ipp>
#include <boost/beast/websocket/detail/hybi13.ipp>
#include <boost/beast/websocket/detail/mask.ipp>
#include <boost/beast/websocket/detail/pmd_extension.ipp>
#include <boost/beast/websocket/detail/prng.ipp>
#include <boost/beast/websocket/detail/service.ipp>
#include <boost/beast/websocket/detail/utf8_checker.ipp>
#include <boost/beast/websocket/impl/error.ipp>
#include <boost/beast/zlib/detail/deflate_stream.ipp>
#include <boost/beast/zlib/detail/inflate_stream.ipp>
#include <boost/beast/zlib/impl/error.ipp>
