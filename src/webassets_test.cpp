#include <crow/app.h>
#include <gmock/gmock.h>
#include <zlib.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <webassets.hpp>
#include "gtest/gtest.h"
using namespace crow;
using namespace std;
using namespace testing;

bool gzipInflate(const std::string& compressedBytes,
                 std::string& uncompressedBytes) {
  if (compressedBytes.size() == 0) {
    uncompressedBytes = compressedBytes;
    return true;
  }

  uncompressedBytes.clear();

  unsigned full_length = compressedBytes.size();
  unsigned half_length = compressedBytes.size() / 2;

  unsigned uncompLength = full_length;
  char* uncomp = (char*)calloc(sizeof(char), uncompLength);

  z_stream strm;
  strm.next_in = (Bytef*)compressedBytes.c_str();
  strm.avail_in = compressedBytes.size();
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false;

  if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK) {
    free(uncomp);
    return false;
  }

  while (!done) {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength) {
      // Increase size of output buffer
      char* uncomp2 = (char*)calloc(sizeof(char), uncompLength + half_length);
      memcpy(uncomp2, uncomp, uncompLength);
      uncompLength += half_length;
      free(uncomp);
      uncomp = uncomp2;
    }

    strm.next_out = (Bytef*)(uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate(&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK) {
      break;
    }
  }

  if (inflateEnd(&strm) != Z_OK) {
    free(uncomp);
    return false;
  }

  for (size_t i = 0; i < strm.total_out; ++i) {
    uncompressedBytes += uncomp[i];
  }
  free(uncomp);
  return true;
}




// Tests static files are loaded correctly
TEST(Webassets, StaticFilesFixedRoutes) {
  std::array<char, 2048> buf;
  SimpleApp app;
  webassets::request_routes(app);
  Server<SimpleApp> server(&app, "127.0.0.1", 45451);
  auto _ = async(launch::async, [&] { server.run(); });

  // Get the homepage
  std::string sendmsg = "GET /\r\n\r\n";

  asio::io_service is;

  asio::ip::tcp::socket c(is);
  c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"),
                                    45451));

  c.send(asio::buffer(sendmsg));

  c.receive(asio::buffer(buf, 2048));
  c.close();

  std::string response(std::begin(buf), std::end(buf));
  // This is a routine to split strings until a newline is hit
  // TODO(ed) this should really use the HTTP parser
  std::vector<std::string> headers;
  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  int content_length = 0;
  std::string content_encoding("");
  while ((pos = response.find("\r\n", prev)) != std::string::npos) {
    auto this_string = response.substr(prev, pos - prev);
    if (this_string == "") {
      prev = pos + 2;
      break;
    }

    if (boost::starts_with(this_string, "Content-Length: ")) {
      content_length = boost::lexical_cast<int>(this_string.substr(16));
      // TODO(ed) This is an unfortunate test, but it's all we have at this
      // point
      // Realistically, the index.html will be more than 500 bytes.  This
      // test will need to be improved at some point
      EXPECT_GT(content_length, 500);
    }
    if (boost::starts_with(this_string, "Content-Encoding: ")) {
      content_encoding = this_string.substr(18);
    }

    headers.push_back(this_string);
    prev = pos + 2;
  }

  auto http_content = response.substr(prev);
  // TODO(ed) ideally the server should support non-compressed gzip assets.
  // Once this
  // occurs, this line will be obsolete
  std::string ungziped_content = http_content;
  if (content_encoding == "gzip") {
    EXPECT_TRUE(gzipInflate(http_content, ungziped_content));
  }

  EXPECT_EQ(headers[0], "HTTP/1.1 200 OK");
  EXPECT_THAT(headers,
              ::testing::Contains("Content-Type: text/html;charset=UTF-8"));

  EXPECT_EQ(ungziped_content.substr(0, 21), "<!DOCTYPE html>\n<html");

  server.stop();
}



// Tests static files are loaded correctly
TEST(Webassets, EtagIsSane) {
  std::array<char, 2048> buf;
  SimpleApp app;
  webassets::request_routes(app);
  Server<SimpleApp> server(&app, "127.0.0.1", 45451);
  auto _ = async(launch::async, [&] { server.run(); });

  // Get the homepage
  std::string sendmsg = "GET /\r\n\r\n";

  asio::io_service is;

  asio::ip::tcp::socket c(is);
  c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"),
                                    45451));

  c.send(asio::buffer(sendmsg));

  c.receive(asio::buffer(buf, 2048));
  c.close();

  std::string response(std::begin(buf), std::end(buf));
  // This is a routine to split strings until a newline is hit
  // TODO(ed) this should really use the HTTP parser
  std::vector<std::string> headers;
  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  int content_length = 0;
  std::string content_encoding("");
  while ((pos = response.find("\r\n", prev)) != std::string::npos) {
    auto this_string = response.substr(prev, pos - prev);
    if (this_string == "") {
      break;
    }

    if (boost::starts_with(this_string, "ETag: ")) {
      auto etag = this_string.substr(6);
      // ETAG should not be blank
      EXPECT_NE(etag, "");
      // SHa1 is 20 characters long
      EXPECT_EQ(etag.size(), 40);
      EXPECT_THAT(etag, MatchesRegex("^[a-f0-9]+$"));
    }

    headers.push_back(this_string);
    prev = pos + 2;
  }

  server.stop();
}