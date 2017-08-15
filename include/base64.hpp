#include <string>

namespace base64 {

bool base64_encode(const std::string &input, std::string &output);
bool base64_decode(const std::string &input, std::string &output);
} // namespace base64