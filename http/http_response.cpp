#include "http_response_class_definition.hpp"



namespace crow
{

boost::beast::http::status Response::result()
{
    return stringResponse->result();
}

}