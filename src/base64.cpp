#include <base64.hpp>

namespace base64 {
bool base64_encode(const std::string &input, std::string &output) {
  // This is left as a raw array (and not a range checked std::array) under the
  // suspicion that the optimizer is not smart enough to remove the range checks
  // that would be done below if at were called.  As is, this array is 64 bytes
  // long, which should be greater than the max of 0b00111111 when indexed
  // NOLINT calls below are to silence clang-tidy about this
  // TODO(ed) this requires further investigation if a more safe method could be
  // used without performance impact.
  static const char encoding_data[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  size_t input_length = input.size();

  // allocate space for output string
  output.clear();
  output.reserve(((input_length + 2) / 3) * 4);

  // for each 3-bytes sequence from the input, extract 4 6-bits sequences and
  // encode using
  // encoding_data lookup table.
  // if input do not contains enough chars to complete 3-byte sequence,use pad
  // char '='
  for (size_t i = 0; i < input_length; i++) {
    int base64code0 = 0;
    int base64code1 = 0;
    int base64code2 = 0;
    int base64code3 = 0;

    base64code0 = (input[i] >> 2) & 0x3f;  // 1-byte 6 bits

    output += encoding_data[base64code0];  // NOLINT
    base64code1 = (input[i] << 4) & 0x3f;  // 1-byte 2 bits +

    if (++i < input_length) {
      base64code1 |= (input[i] >> 4) & 0x0f;  // 2-byte 4 bits
      output += encoding_data[base64code1];   // NOLINT
      base64code2 = (input[i] << 2) & 0x3f;   // 2-byte 4 bits +

      if (++i < input_length) {
        base64code2 |= (input[i] >> 6) & 0x03;  // 3-byte 2 bits
        base64code3 = input[i] & 0x3f;          // 3-byte 6 bits
        output += encoding_data[base64code2];   // NOLINT
        output += encoding_data[base64code3];   // NOLINT
      } else {
        output += encoding_data[base64code2];  // NOLINT
        output += '=';
      }
    } else {
      output += encoding_data[base64code1];  // NOLINT
      output += '=';
      output += '=';
    }
  }

  return true;
}

bool base64_decode(const std::string &input, std::string &output) {
  static const char nop = -1;
  // See note on encoding_data[] in above function
  static const char decoding_data[] = {
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, 62,  nop,
      nop, nop, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  nop, nop,
      nop, nop, nop, nop, nop, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
      10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
      25,  nop, nop, nop, nop, nop, nop, 26,  27,  28,  29,  30,  31,  32,  33,
      34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
      49,  50,  51,  nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
      nop};

  size_t input_length = input.size();

  // allocate space for output string
  output.clear();
  output.reserve(((input_length + 2) / 3) * 4);

  // for each 4-bytes sequence from the input, extract 4 6-bits sequences by
  // droping first two bits
  // and regenerate into 3 8-bits sequences

  for (size_t i = 0; i < input_length; i++) {
    char base64code0;
    char base64code1;
    char base64code2 = 0;  // initialized to 0 to suppress warnings
    char base64code3;

    base64code0 = decoding_data[static_cast<int>(input[i])];  // NOLINT
    if (base64code0 == nop) {  // non base64 character
      return false;
    }
    if (!(++i < input_length)) {  // we need at least two input bytes for first
                                  // byte output
      return false;
    }
    base64code1 = decoding_data[static_cast<int>(input[i])];  // NOLINT
    if (base64code1 == nop) {  // non base64 character
      return false;
    }
    output += static_cast<char>((base64code0 << 2) | ((base64code1 >> 4) & 0x3));

    if (++i < input_length) {
      char c = input[i];
      if (c == '=') {  // padding , end of input
        return (base64code1 & 0x0f) == 0;
      }
      base64code2 = decoding_data[static_cast<int>(input[i])];  // NOLINT
      if (base64code2 == nop) {  // non base64 character
        return false;
      }
      output += static_cast<char>(((base64code1 << 4) & 0xf0) | ((base64code2 >> 2) & 0x0f));
    }

    if (++i < input_length) {
      char c = input[i];
      if (c == '=') {  // padding , end of input
        return (base64code2 & 0x03) == 0;
      }
      base64code3 = decoding_data[static_cast<int>(input[i])];  // NOLINT
      if (base64code3 == nop) {  // non base64 character
        return false;
      }
      output += static_cast<char>((((base64code2 << 6) & 0xc0) | base64code3));
    }
  }

  return true;
}
}  // namespace base64
