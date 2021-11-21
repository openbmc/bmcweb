#include "http_request_class_decl.hpp"

#include <boost/beast/http/string_body.hpp>

namespace crow
{
struct RequestImpl : public Request
{
  ~RequestImpl
  boost::beast::http::request<boost::beast::http::string_body> req;
  boost::beast::http::fields& fields;
  std::string_view url_{};
  std::shared_ptr<persistent_data::UserSession> session_;

  std::string_view url() const { return url_; }
  std::shared_ptr<persistent_data::UserSession> session() const { return session_; }
}

}