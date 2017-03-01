#include <base64.hpp>
#include <cassert>

namespace base64
{
bool base64_encode(const gsl::cstring_span<> &input, std::string &output)
{
    static const char encoding_data[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    unsigned int input_length = input.size();

    // allocate space for output string
    output.clear();
    output.reserve(((input_length + 2) / 3) * 4);

    // for each 3-bytes sequence from the input, extract 4 6-bits sequences and
    // encode using
    // encoding_data lookup table.
    // if input do not contains enough chars to complete 3-byte sequence,use pad
    // char '='
    for (unsigned int i = 0; i < input_length; i++) {
        int base64code0 = 0;
        int base64code1 = 0;
        int base64code2 = 0;
        int base64code3 = 0;

        base64code0 = (input[i] >> 2) & 0x3f;  // 1-byte 6 bits
        output += encoding_data[base64code0];
        base64code1 = (input[i] << 4) & 0x3f;  // 1-byte 2 bits +

        if (++i < input_length) {
            base64code1 |= (input[i] >> 4) & 0x0f;  // 2-byte 4 bits
            output += encoding_data[base64code1];
            base64code2 = (input[i] << 2) & 0x3f;  // 2-byte 4 bits +

            if (++i < input_length) {
                base64code2 |= (input[i] >> 6) & 0x03;  // 3-byte 2 bits
                base64code3 = input[i] & 0x3f;          // 3-byte 6 bits
                output += encoding_data[base64code2];
                output += encoding_data[base64code3];
            } else {
                output += encoding_data[base64code2];
                output += '=';
            }
        } else {
            output += encoding_data[base64code1];
            output += '=';
            output += '=';
        }
    }

    return true;
}


bool base64_decode(const gsl::cstring_span<> &input, std::string &output) 
{
  static const char nop = -1;
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

  unsigned int input_length = input.size();

  // allocate space for output string
  output.clear();
  output.reserve(((input_length + 2) / 3) * 4);

  // for each 4-bytes sequence from the input, extract 4 6-bits sequences by
  // droping first two bits
  // and regenerate into 3 8-bits sequence

  for (unsigned int i = 0; i < input_length; i++) {
    char base64code0;
    char base64code1;
    char base64code2 = 0; // initialized to 0 to suppress warnings
    char base64code3;

    base64code0 = decoding_data[static_cast<int>(input[i])];
    if (base64code0 == nop) // non base64 character
      return false;
    if (!(++i < input_length)) // we need at least two input bytes for first
                               // byte output
      return false;
    base64code1 = decoding_data[static_cast<int>(input[i])];
    if (base64code1 == nop) // non base64 character
      return false;

    output += ((base64code0 << 2) | ((base64code1 >> 4) & 0x3));

    if (++i < input_length) {
      char c = input[i];
      if (c == '=') { // padding , end of input
        assert((base64code1 & 0x0f) == 0);
        return true;
      }
      base64code2 = decoding_data[static_cast<int>(input[i])];
      if (base64code2 == nop) // non base64 character
        return false;

      output += ((base64code1 << 4) & 0xf0) | ((base64code2 >> 2) & 0x0f);
    }

    if (++i < input_length) {
      char c = input[i];
      if (c == '=') { // padding , end of input
        assert((base64code2 & 0x03) == 0);
        return true;
      }
      base64code3 = decoding_data[static_cast<int>(input[i])];
      if (base64code3 == nop) // non base64 character
        return false;

      output += (((base64code2 << 6) & 0xc0) | base64code3);
    }
  }

  return true;
}

}