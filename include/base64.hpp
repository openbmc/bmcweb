#include <gsl/string_span>
#include <string>

namespace base64 {

bool base64_encode(const gsl::cstring_span<> &input, std::string &output);
bool base64_decode(const gsl::cstring_span<> &input, std::string &output);
}