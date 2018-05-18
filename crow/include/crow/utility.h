#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include "nlohmann/json.hpp"
#include <boost/utility/string_view.hpp>

namespace crow {
namespace black_magic {
struct OutOfRange {
  OutOfRange(unsigned /*pos*/, unsigned /*length*/) {}
};
constexpr unsigned requires_in_range(unsigned i, unsigned len) {
  return i >= len ? throw OutOfRange(i, len) : i;
}

class const_str {
  const char* const begin_;
  unsigned size_;

 public:
  template <unsigned N>
  constexpr const_str(const char (&arr)[N]) : begin_(arr), size_(N - 1) {
    static_assert(N >= 1, "not a string literal");
  }
  constexpr char operator[](unsigned i) const {
    return requires_in_range(i, size_), begin_[i];
  }

  constexpr operator const char*() const { return begin_; }

  constexpr const char* begin() const { return begin_; }
  constexpr const char* end() const { return begin_ + size_; }

  constexpr unsigned size() const { return size_; }
};

constexpr unsigned find_closing_tag(const_str s, unsigned p) {
  return s[p] == '>' ? p : find_closing_tag(s, p + 1);
}

constexpr bool is_valid(const_str s, unsigned i = 0, int f = 0) {
  return i == s.size()
             ? f == 0
             : f < 0 || f >= 2
                   ? false
                   : s[i] == '<' ? is_valid(s, i + 1, f + 1)
                                 : s[i] == '>' ? is_valid(s, i + 1, f - 1)
                                               : is_valid(s, i + 1, f);
}

constexpr bool is_equ_p(const char* a, const char* b, unsigned n) {
  return *a == 0 && *b == 0 && n == 0
             ? true
             : (*a == 0 || *b == 0)
                   ? false
                   : n == 0 ? true
                            : *a != *b ? false : is_equ_p(a + 1, b + 1, n - 1);
}

constexpr bool is_equ_n(const_str a, unsigned ai, const_str b, unsigned bi,
                        unsigned n) {
  return ai + n > a.size() || bi + n > b.size()
             ? false
             : n == 0 ? true
                      : a[ai] != b[bi] ? false
                                       : is_equ_n(a, ai + 1, b, bi + 1, n - 1);
}

constexpr bool is_int(const_str s, unsigned i) {
  return is_equ_n(s, i, "<int>", 0, 5);
}

constexpr bool is_uint(const_str s, unsigned i) {
  return is_equ_n(s, i, "<uint>", 0, 6);
}

constexpr bool is_float(const_str s, unsigned i) {
  return is_equ_n(s, i, "<float>", 0, 7) || is_equ_n(s, i, "<double>", 0, 8);
}

constexpr bool is_str(const_str s, unsigned i) {
  return is_equ_n(s, i, "<str>", 0, 5) || is_equ_n(s, i, "<string>", 0, 8);
}

constexpr bool is_path(const_str s, unsigned i) {
  return is_equ_n(s, i, "<path>", 0, 6);
}

template <typename T>
struct parameter_tag {
  static const int value = 0;
};
#define CROW_INTERNAL_PARAMETER_TAG(t, i) \
  template <>                             \
  struct parameter_tag<t> {               \
    static const int value = i;           \
  }
CROW_INTERNAL_PARAMETER_TAG(int, 1);
CROW_INTERNAL_PARAMETER_TAG(char, 1);
CROW_INTERNAL_PARAMETER_TAG(short, 1);
CROW_INTERNAL_PARAMETER_TAG(long, 1);
CROW_INTERNAL_PARAMETER_TAG(long long, 1);
CROW_INTERNAL_PARAMETER_TAG(unsigned int, 2);
CROW_INTERNAL_PARAMETER_TAG(unsigned char, 2);
CROW_INTERNAL_PARAMETER_TAG(unsigned short, 2);
CROW_INTERNAL_PARAMETER_TAG(unsigned long, 2);
CROW_INTERNAL_PARAMETER_TAG(unsigned long long, 2);
CROW_INTERNAL_PARAMETER_TAG(double, 3);
CROW_INTERNAL_PARAMETER_TAG(std::string, 4);
#undef CROW_INTERNAL_PARAMETER_TAG
template <typename... Args>
struct compute_parameter_tag_from_args_list;

template <>
struct compute_parameter_tag_from_args_list<> {
  static const int value = 0;
};

template <typename Arg, typename... Args>
struct compute_parameter_tag_from_args_list<Arg, Args...> {
  static const int sub_value =
      compute_parameter_tag_from_args_list<Args...>::value;
  static const int value =
      parameter_tag<typename std::decay<Arg>::type>::value
          ? sub_value * 6 + parameter_tag<typename std::decay<Arg>::type>::value
          : sub_value;
};

static inline bool is_parameter_tag_compatible(uint64_t a, uint64_t b) {
  if (a == 0) {
    return b == 0;
  }
  if (b == 0) {
    return a == 0;
  }
  int sa = a % 6;
  int sb = a % 6;
  if (sa == 5) {
    sa = 4;
  }
  if (sb == 5) {
    sb = 4;
  }
  if (sa != sb) {
    return false;
  }
  return is_parameter_tag_compatible(a / 6, b / 6);
}

static inline unsigned find_closing_tag_runtime(const char* s, unsigned p) {
  return s[p] == 0 ? throw std::runtime_error("unmatched tag <")
                   : s[p] == '>' ? p : find_closing_tag_runtime(s, p + 1);
}

static inline uint64_t get_parameter_tag_runtime(const char* s,
                                                 unsigned p = 0) {
  return s[p] == 0
             ? 0
             : s[p] == '<'
                   ? (std::strncmp(s + p, "<int>", 5) == 0
                          ? get_parameter_tag_runtime(
                                s, find_closing_tag_runtime(s, p)) *
                                    6 +
                                1
                          : std::strncmp(s + p, "<uint>", 6) == 0
                                ? get_parameter_tag_runtime(
                                      s, find_closing_tag_runtime(s, p)) *
                                          6 +
                                      2
                                : (std::strncmp(s + p, "<float>", 7) == 0 ||
                                   std::strncmp(s + p, "<double>", 8) == 0)
                                      ? get_parameter_tag_runtime(
                                            s, find_closing_tag_runtime(s, p)) *
                                                6 +
                                            3
                                      : (std::strncmp(s + p, "<str>", 5) == 0 ||
                                         std::strncmp(s + p, "<string>", 8) ==
                                             0)
                                            ? get_parameter_tag_runtime(
                                                  s, find_closing_tag_runtime(
                                                         s, p)) *
                                                      6 +
                                                  4
                                            : std::strncmp(s + p, "<path>",
                                                           6) == 0
                                                  ? get_parameter_tag_runtime(
                                                        s,
                                                        find_closing_tag_runtime(
                                                            s, p)) *
                                                            6 +
                                                        5
                                                  : throw std::runtime_error(
                                                        "invalid parameter "
                                                        "type"))
                   : get_parameter_tag_runtime(s, p + 1);
}

constexpr uint64_t get_parameter_tag(const_str s, unsigned p = 0) {
  return p == s.size()
             ? 0
             : s[p] == '<'
                   ? (is_int(s, p)
                          ? get_parameter_tag(s, find_closing_tag(s, p)) * 6 + 1
                          : is_uint(s, p)
                                ? get_parameter_tag(s, find_closing_tag(s, p)) *
                                          6 +
                                      2
                                : is_float(s, p)
                                      ? get_parameter_tag(
                                            s, find_closing_tag(s, p)) *
                                                6 +
                                            3
                                      : is_str(s, p)
                                            ? get_parameter_tag(
                                                  s, find_closing_tag(s, p)) *
                                                      6 +
                                                  4
                                            : is_path(s, p)
                                                  ? get_parameter_tag(
                                                        s, find_closing_tag(
                                                               s, p)) *
                                                            6 +
                                                        5
                                                  : throw std::runtime_error(
                                                        "invalid parameter "
                                                        "type"))
                   : get_parameter_tag(s, p + 1);
}

template <typename... T>
struct S {
  template <typename U>
  using push = S<U, T...>;
  template <typename U>
  using push_back = S<T..., U>;
  template <template <typename... Args> class U>
  using rebind = U<T...>;
};
template <typename F, typename Set>
struct CallHelper;
template <typename F, typename... Args>
struct CallHelper<F, S<Args...>> {
  template <typename F1, typename... Args1,
            typename = decltype(std::declval<F1>()(std::declval<Args1>()...))>
  static char __test(int);

  template <typename...>
  static int __test(...);

  static constexpr bool value = sizeof(__test<F, Args...>(0)) == sizeof(char);
};

template <int N>
struct single_tag_to_type {};

template <>
struct single_tag_to_type<1> {
  using type = int64_t;
};

template <>
struct single_tag_to_type<2> {
  using type = uint64_t;
};

template <>
struct single_tag_to_type<3> {
  using type = double;
};

template <>
struct single_tag_to_type<4> {
  using type = std::string;
};

template <>
struct single_tag_to_type<5> {
  using type = std::string;
};

template <uint64_t Tag>
struct arguments {
  using subarguments = typename arguments<Tag / 6>::type;
  using type = typename subarguments::template push<
      typename single_tag_to_type<Tag % 6>::type>;
};

template <>
struct arguments<0> {
  using type = S<>;
};

template <typename... T>
struct last_element_type {
  using type =
      typename std::tuple_element<sizeof...(T) - 1, std::tuple<T...>>::type;
};

template <>
struct last_element_type<> {};

// from
// http://stackoverflow.com/questions/13072359/c11-compile-time-array-with-logarithmic-evaluation-depth
template <class T>
using Invoke = typename T::type;

template <unsigned...>
struct seq {
  using type = seq;
};

template <class S1, class S2>
struct concat;

template <unsigned... I1, unsigned... I2>
struct concat<seq<I1...>, seq<I2...>> : seq<I1..., (sizeof...(I1) + I2)...> {};

template <class S1, class S2>
using Concat = Invoke<concat<S1, S2>>;

template <unsigned N>
struct gen_seq;
template <unsigned N>
using GenSeq = Invoke<gen_seq<N>>;

template <unsigned N>
struct gen_seq : Concat<GenSeq<N / 2>, GenSeq<N - N / 2>> {};

template <>
struct gen_seq<0> : seq<> {};
template <>
struct gen_seq<1> : seq<0> {};

template <typename Seq, typename Tuple>
struct pop_back_helper;

template <unsigned... N, typename Tuple>
struct pop_back_helper<seq<N...>, Tuple> {
  template <template <typename... Args> class U>
  using rebind = U<typename std::tuple_element<N, Tuple>::type...>;
};

template <typename... T>
struct pop_back  //: public pop_back_helper<typename
                 // gen_seq<sizeof...(T)-1>::type, std::tuple<T...>>
{
  template <template <typename... Args> class U>
  using rebind =
      typename pop_back_helper<typename gen_seq<sizeof...(T) - 1>::type,
                               std::tuple<T...>>::template rebind<U>;
};

template <>
struct pop_back<> {
  template <template <typename... Args> class U>
  using rebind = U<>;
};

// from
// http://stackoverflow.com/questions/2118541/check-if-c0x-parameter-pack-contains-a-type
template <typename Tp, typename... List>
struct contains : std::true_type {};

template <typename Tp, typename Head, typename... Rest>
struct contains<Tp, Head, Rest...>
    : std::conditional<std::is_same<Tp, Head>::value, std::true_type,
                       contains<Tp, Rest...>>::type {};

template <typename Tp>
struct contains<Tp> : std::false_type {};

template <typename T>
struct empty_context {};

template <typename T>
struct promote {
  using type = T;
};

#define CROW_INTERNAL_PROMOTE_TYPE(t1, t2) \
  template <>                              \
  struct promote<t1> {                     \
    using type = t2;                       \
  }

CROW_INTERNAL_PROMOTE_TYPE(char, int64_t);
CROW_INTERNAL_PROMOTE_TYPE(short, int64_t);
CROW_INTERNAL_PROMOTE_TYPE(int, int64_t);
CROW_INTERNAL_PROMOTE_TYPE(long, int64_t);
CROW_INTERNAL_PROMOTE_TYPE(long long, int64_t);
CROW_INTERNAL_PROMOTE_TYPE(unsigned char, uint64_t);
CROW_INTERNAL_PROMOTE_TYPE(unsigned short, uint64_t);
CROW_INTERNAL_PROMOTE_TYPE(unsigned int, uint64_t);
CROW_INTERNAL_PROMOTE_TYPE(unsigned long, uint64_t);
CROW_INTERNAL_PROMOTE_TYPE(unsigned long long, uint64_t);
CROW_INTERNAL_PROMOTE_TYPE(float, double);
#undef CROW_INTERNAL_PROMOTE_TYPE

template <typename T>
using promote_t = typename promote<T>::type;

}  // namespace black_magic

namespace detail {

template <class T, std::size_t N, class... Args>
struct get_index_of_element_from_tuple_by_type_impl {
  static constexpr auto value = N;
};

template <class T, std::size_t N, class... Args>
struct get_index_of_element_from_tuple_by_type_impl<T, N, T, Args...> {
  static constexpr auto value = N;
};

template <class T, std::size_t N, class U, class... Args>
struct get_index_of_element_from_tuple_by_type_impl<T, N, U, Args...> {
  static constexpr auto value =
      get_index_of_element_from_tuple_by_type_impl<T, N + 1, Args...>::value;
};

}  // namespace detail

namespace utility {
template <class T, class... Args>
T& get_element_by_type(std::tuple<Args...>& t) {
  return std::get<detail::get_index_of_element_from_tuple_by_type_impl<
      T, 0, Args...>::value>(t);
}

template <typename T>
struct function_traits;

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {
  using parent_t = function_traits<decltype(&T::operator())>;
  static const size_t arity = parent_t::arity;
  using result_type = typename parent_t::result_type;
  template <size_t i>
  using arg = typename parent_t::template arg<i>;
};

template <typename ClassType, typename R, typename... Args>
struct function_traits<R (ClassType::*)(Args...) const> {
  static const size_t arity = sizeof...(Args);

  using result_type = R;

  template <size_t i>
  using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

template <typename ClassType, typename R, typename... Args>
struct function_traits<R (ClassType::*)(Args...)> {
  static const size_t arity = sizeof...(Args);

  using result_type = R;

  template <size_t i>
  using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
  static const size_t arity = sizeof...(Args);

  using result_type = R;

  template <size_t i>
  using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

inline static std::string base64encode(
    const char* data, size_t size,
    const char* key =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/") {
  std::string ret;
  ret.resize((size + 2) / 3 * 4);
  auto it = ret.begin();
  while (size >= 3) {
    *it++ = key[(((unsigned char)*data) & 0xFC) >> 2];
    unsigned char h = (((unsigned char)*data++) & 0x03) << 4;
    *it++ = key[h | ((((unsigned char)*data) & 0xF0) >> 4)];
    h = (((unsigned char)*data++) & 0x0F) << 2;
    *it++ = key[h | ((((unsigned char)*data) & 0xC0) >> 6)];
    *it++ = key[((unsigned char)*data++) & 0x3F];

    size -= 3;
  }
  if (size == 1) {
    *it++ = key[(((unsigned char)*data) & 0xFC) >> 2];
    unsigned char h = (((unsigned char)*data++) & 0x03) << 4;
    *it++ = key[h];
    *it++ = '=';
    *it++ = '=';
  } else if (size == 2) {
    *it++ = key[(((unsigned char)*data) & 0xFC) >> 2];
    unsigned char h = (((unsigned char)*data++) & 0x03) << 4;
    *it++ = key[h | ((((unsigned char)*data) & 0xF0) >> 4)];
    h = (((unsigned char)*data++) & 0x0F) << 2;
    *it++ = key[h];
    *it++ = '=';
  }
  return ret;
}

inline static std::string base64encode_urlsafe(const char* data, size_t size) {
  return base64encode(
      data, size,
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");
}

// TODO this is temporary and should be deleted once base64 is refactored out of
// crow
inline bool base64_decode(const boost::string_view input, std::string& output) {
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
    output +=
        static_cast<char>((base64code0 << 2) | ((base64code1 >> 4) & 0x3));

    if (++i < input_length) {
      char c = input[i];
      if (c == '=') {  // padding , end of input
        return (base64code1 & 0x0f) == 0;
      }
      base64code2 = decoding_data[static_cast<int>(input[i])];  // NOLINT
      if (base64code2 == nop) {  // non base64 character
        return false;
      }
      output += static_cast<char>(((base64code1 << 4) & 0xf0) |
                                  ((base64code2 >> 2) & 0x0f));
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

}  // namespace utility
}  // namespace crow
