#ifndef C10_UTIL_STRINGUTIL_H_
#define C10_UTIL_STRINGUTIL_H_

#include <c10/macros/Macros.h>
#include <c10/util/string_utils.h>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace c10 {

namespace detail {

// Obtains the base name from a full path.
C10_API std::string StripBasename(const std::string& full_path);

template <typename T>
struct CanonicalizeStrTypes {
  using type = const T&;
};

template <size_t N>
struct CanonicalizeStrTypes<char[N]> {
  using type = const char *;
};


inline std::ostream& _str(std::ostream& ss) {
  return ss;
}

template <typename T>
inline std::ostream& _str(std::ostream& ss, const T& t) {
  ss << t;
  return ss;
}

template <typename T, typename... Args>
inline std::ostream& _str(std::ostream& ss, const T& t, const Args&... args) {
  return _str(_str(ss, t), args...);
}

template<typename... Args>
struct _str_wrapper final {
  static std::string call(const Args&... args) {
    std::ostringstream ss;
    _str(ss, args...);
    return ss.str();
  }
};

// Specializations for already-a-string types.
template<>
struct _str_wrapper<std::string> final {
  // return by reference to avoid the binary size of a string copy
  static const std::string& call(const std::string& str) {
    return str;
  }
};

template<>
struct _str_wrapper<const char*> final {
  static std::string call(const char* str) {
    return str;
  }
};

// For c10::str() with an empty argument list (which is common in our assert macros),
// we don't want to pay the binary size for constructing and destructing a stringstream
// or even constructing a string. Let's just return a reference to an empty string.
template<>
struct _str_wrapper<> final {
  static const std::string& call() {
    thread_local const std::string empty_string_literal;
    return empty_string_literal;
  }
};

} // namespace detail

// Convert a list of string-like arguments into a single string.
template <typename... Args>
inline decltype(auto) str(const Args&... args) {
  return detail::_str_wrapper<typename detail::CanonicalizeStrTypes<Args>::type...>::call(args...);
}

template <class Container>
inline std::string Join(const std::string& delimiter, const Container& v) {
  std::stringstream s;
  int cnt = static_cast<int64_t>(v.size()) - 1;
  for (auto i = v.begin(); i != v.end(); ++i, --cnt) {
    s << (*i) << (cnt ? delimiter : "");
  }
  return s.str();
}

// Replace all occurrences of "from" substring to "to" string.
// Returns number of replacements
size_t C10_API ReplaceAll(std::string& s, const char* from, const char* to);

/// Represents a location in source code (for debugging).
struct C10_API SourceLocation {
  const char* function;
  const char* file;
  uint32_t line;
};

std::ostream& operator<<(std::ostream& out, const SourceLocation& loc);

// unix isprint but insensitive to locale
inline static bool isPrint(char s) {
  return s > 0x1f && s < 0x7f;
}

inline void printQuotedString(std::ostream& stmt, const std::string& str) {
  stmt << "\"";
  for (auto s : str) {
    switch (s) {
      case '\\':
        stmt << "\\\\";
        break;
      case '\'':
        stmt << "\\'";
        break;
      case '\"':
        stmt << "\\\"";
        break;
      case '\a':
        stmt << "\\a";
        break;
      case '\b':
        stmt << "\\b";
        break;
      case '\f':
        stmt << "\\f";
        break;
      case '\n':
        stmt << "\\n";
        break;
      case '\r':
        stmt << "\\r";
        break;
      case '\t':
        stmt << "\\t";
        break;
      case '\v':
        stmt << "\\v";
        break;
      default:
        if (isPrint(s)) {
          stmt << s;
        } else {
          // C++ io has stateful formatting settings. Messing with
          // them is probably worse than doing this manually.
          char buf[4] = "000";
          buf[2] += s % 8;
          s /= 8;
          buf[1] += s % 8;
          s /= 8;
          buf[0] += s;
          stmt << "\\" << buf;
        }
        break;
    }
  }
  stmt << "\"";
}

} // namespace c10

#endif // C10_UTIL_STRINGUTIL_H_
