//
// Created by finni on 14/07/2024.
//

#include "json.hpp"
#include <stack>
#include <utility>

using namespace Json;

// JsonObject

#define IS_JSON_TYPE(TypeAlias) (std::is_same_v<TypeAlias, Null> || std::is_same_v<TypeAlias, Boolean> || std::is_same_v<TypeAlias, Number> || std::is_same_v<TypeAlias, String> || std::is_same_v<TypeAlias, List> || std::is_same_v<TypeAlias, Map>)
#define ASSERT_JSON_TYPE(TypeAlias) static_assert(IS_JSON_TYPE(TypeAlias), "Incompatible JSON type");

JsonObject::JsonObject() = default;

template<typename T>
JsonObject::JsonObject(T data) {
  constexpr bool isJsonType{IS_JSON_TYPE(T)};
  static_assert(isJsonType, "Incompatible JSON type");
  
  value = data;
}

template<typename T>
[[maybe_unused]] inline T *JsonObject::as() {
  ASSERT_JSON_TYPE(T)
  return std::get_if<T>(&value);
}

template<typename T>
JsonObject& JsonObject::operator=(T data) {

}

// JsonNumber
// BadNumberException

inline JsonNumber::Exception::BadNumberException() = default;

JsonNumber::BadNumberException::BadNumberException(std::string &&str) {
  message += std::forward<std::string>(str);
}

JsonNumber::Exception::~BadNumberException() = default;

const char *JsonNumber::Exception::what() const noexcept {
  return message.c_str();
}

// JsonNumber

JsonNumber::JsonNumber(std::string &&str) : str(std::forward<std::string>(str)) {}

template<typename IntT>
IntT JsonNumber::asInteger() {
  IntT num(str[0] - '0');
  
  for (int i = 1; i < str.length(); ++i) {
    num = (num * 10) + (str[i] - '0');
  }
  
  return num;
}

const std::string &JsonNumber::asString() {
  return str;
}

constexpr void JsonNumber::assert() {
  if (str.empty())
    throw BadNumberException("Expected ");
  
  int index{0};
  
  if (str[0] == '-')
    index = 1;
  
  for (; index < str.length(); ++index) {
    switch (str[index]) {
      case '.':
        ++index;
        goto decimalLabel;
      case 'e':
        ++index;
        goto exponentLabel;
    }
    
    if (std::isdigit(str[index])) {
      continue;
    }
    
    throw BadNumberException();
  }
  
  exponentLabel:
  if (index >= str.length())
    throw BadNumberException();
  else if (str[index] == '-' || str[index] == '+')
    ++index;
  
  for (; index < str.length(); ++index) {
    if (std::isdigit(str[index])) {
      continue;
    }
    
    throw BadNumberException();
  }
  
  decimalLabel:
  if (index >= str.length() || !std::isdigit(str[index])) {
    throw BadNumberException();
  }
  
  ++index;
  
  for (; index < str.length(); ++index) {
    if (str[index] == 'e') {
      goto exponentLabel;
    } else if (std::isdigit(str[index])) {
      continue;
    }
    
    throw BadNumberException();
  }
}

template<typename DecimalT>
DecimalT JsonNumber::asDecimal() {
  int index{0};
  bool neg{false};
  
  if (str[0] == '-') {
    neg = true;
    ++index;
  }
  
  DecimalT num(str[index] - '0');
  ++index;
  
  for (; index < str.length(); ++index) {
    switch (str[index]) {
      case 'e':
        goto exponentLabel;
      case '.':
        goto decimalLabel;
      default:
        num = (num * 10) + (str[index] - '0');
    }
  }
  
  return num;
  
  exponentLabel:
  {
    ++index;
    bool negEx{false};
    
    if (str[index] == '+') {
      ++index;
    } else if (str[index] == '-') {
      ++index;
      negEx = true;
    }
    
    double exponent(str[index] - '0');
    ++index;
    
    for (; index < str.length(); ++index) {
      exponent = (exponent * 10) + (str[index] - '0');
    }
    
    return std::pow(neg ? -num : num, negEx ? -exponent : exponent);
  }
  
  decimalLabel:
  {
    ++index;
    double unit = 10;
    
    for (; index < str.length() - 1; ++index) {
      if (str[index] == 'e') goto exponentLabel;
      
      num += (str[index] - '0') / unit;
      unit *= 10;
    }
    
    num += (str[index] - '0') / unit;
    
    return neg ? -num : num;
  }
}

// JsonString

inline JsonString::JsonString(std::string &&str) : std::string{std::move(str)} {}

inline JsonString::JsonString(std::string &str) : std::string{std::move(str)} {}

inline JsonString::JsonString(char *str) : std::string(str) {}

[[maybe_unused]] inline void JsonString::lower() {
  std::transform(begin(), end(), begin(), [](unsigned char c) { return std::tolower(c); });
}

[[maybe_unused]] inline void JsonString::upper() {
  std::transform(begin(), end(), begin(), [](unsigned char c) { return std::toupper(c); });
}

[[maybe_unused]] inline std::string &JsonString::basicString() {
  return *static_cast<std::string *>(this);
}

// JsonList

// JsonMap

// JsonBoolean

inline JsonBoolean::JsonBoolean(bool state) : value(state) {}

inline JsonBoolean::operator bool() const {
  return value;
}

inline JsonBoolean &JsonBoolean::operator=(bool newValue) {
  value = newValue;
  return *this;
}

// Parser
// ParsingException

Parser::Exception::ParsingException() = default;

Parser::Exception::ParsingException(const std::string &information) {
  message.push_back('\n');
  message += information;
}

const char *Parser::Exception::what() const noexcept {
  return message.c_str();
}

// Parser

void Parser::parse(JsonObject &JsonObject, const std::string &jsonString) {
  auto &&iterator = jsonString.begin(), &&end = jsonString.end();
  interpretValue(iterator, end, JsonObject);
}

void Parser::interpretValue(const_string_it &iterator, const_string_it end, JsonObject &JsonObject) {
  if (!findFirstNonWhitespace(iterator, end)) {
    return;
  }
  
  char c = *iterator;
  
  switch (c) {
    case '"':
      return parseString(iterator, end, JsonObject);
    case '{':
      return parseMap(iterator, end, JsonObject);
    case '[':
      return parseList(iterator, end, JsonObject);
    case '-':
      return parseNumber(iterator, end, JsonObject);
    case 't':
      return parseTrue(iterator, end, JsonObject);
    case 'f':
      return parseFalse(iterator, end, JsonObject);
    case 'n':
      return parseNull(iterator, end, JsonObject);
    default:
      if (std::isdigit(c)) return parseNumber(iterator, end, JsonObject);
      else throw ParsingException("Unknown token");
  }
}

bool Parser::iteratorAligns(const_string_it &iterator, const_string_it end, const char *s) {
  if (s.length() > (end - iterator)) {
    return false;
  }
  
  for (int i = 0; i < s.length(); ++i) {
    if (*iterator == s[i]) ++iterator;
    else return false;
  }
  
  ++iterator;
  return true;
}

inline bool Parser::isWhitespaceOrComma(char c) {
  switch (c) {
    case ',':
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
    default:
      return false;
  }
}

inline bool Parser::isWhitespace(char c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
    default:
      return false;
  }
}

char Parser::findFirstNonWhitespace(Json::const_string_it &iterator, Json::const_string_it end) {
  for (; iterator != end; ++iterator) {
    if (!isWhitespace(*iterator)) {
      return *iterator;
    }
  }
  
  return ('\0');
}

void Parser::parseNull(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject &JsonObject) {
  if (iteratorAligns(iterator, end, "null")) {
    JsonObject = JsonNull();
  } else {
    throw ParsingException("Unknown value");
  }
}

void Parser::parseTrue(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject *JsonObject) {
  if (iteratorAligns(iterator, end, "true")) {
    JsonObject = new JsonObject{true};
  } else {
    throw ParsingException("Unknown value");
  }
}

void Parser::parseFalse(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject *JsonObject) {
  if (iteratorAligns(iterator, end, "false")) {
    JsonObject = new JsonObject{new JsonBoolean{false}};
  } else {
    throw ParsingException("Unknown value");
  }
}

void Parser::parseNumber(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject *JsonObject) {
  constexpr char MALFORMED_NUMBER[] = "Malformed number, unknown token found when parsing number";
  
  std::string str{};
  
  for (; iterator != end; ++iterator) {
    switch (*iterator) {
      case '.':
        goto decimalLabel;
      case 'e':
        goto exponentLabel;
    }
    
    if (std::isdigit(*iterator)) str.push_back(*iterator);
    else throw ParsingException(MALFORMED_NUMBER);
  }
  
  decimalLabel:
  {
    if (!std::isdigit(*++iterator)) throw ParsingException(MALFORMED_NUMBER);
    
    str.push_back('.');
    str.push_back(*iterator);
    ++iterator;
    
    for (; iterator != end; ++iterator) {
      if (std::isdigit(*iterator)) str.push_back(*iterator);
      else if (*iterator == 'e') goto exponentLabel;
      else if (isWhitespaceOrComma(*iterator))
        JsonObject = new JsonObject(str);
      else throw ParsingException(MALFORMED_NUMBER);
    }
  }
  
  exponentLabel:
  {
    ++iterator;
    
    switch (*iterator) {
      case '-':
        str.push_back('-');
      case '+':
        ++iterator;
    }
    
    if (iterator == end) {
      throw ParsingException("Malformed number");
    }
    
    do {
      if (std::isdigit(*iterator))
        str.push_back(*iterator);
      else if (*iterator == 'e') {
        str.push_back('e');
      }
      ++iterator;
    } while (iterator != end);
    
    
  }
}

void Parser::parseString(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject *JsonObject) {
  std::string str;
  ++iterator; // dereferenced iterator should be \"
  bool backslash{false};
  
  for (; iterator != end; ++iterator) {
    if (backslash) {
      backslash = false;
      str.push_back(*iterator);
      continue;
    }
    
    switch (*iterator) {
      case '\"': {
        JsonObject = new JsonObject(new JsonString(str));
        return;
      }
      case '\\': {
        backslash = true;
        str.push_back('\\');
        break;
      }
      default: {
        str.push_back(*iterator);
      }
    }
  }
  
  throw ParsingException("Malformed string");
}

void Parser::parseList(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject *JsonObject) {
  ++iterator; // iterator is [
  constexpr static char UNFINISHED_LIST[] = "Unfinished list, expected ] after [ in list";
  
  if (!findFirstNonWhitespace(iterator, end))
    throw ParsingException(UNFINISHED_LIST);
  
  auto *list = new JsonList();
  JsonObject = new JsonObject(list);
  
  while (*iterator != ']') {
    if (!findFirstNonWhitespace(iterator, end))
      throw ParsingException(UNFINISHED_LIST);
    
    JsonObject element;
    Parser::interpretValue(iterator, end, element);
    list->emplace(*element);
  }
}

void Parser::parseMap(Json::const_string_it &iterator, Json::const_string_it end, Json::JsonObject *JsonObject) {
  constexpr static char UNFINISHED_MAP[] = "Unfinished map, expected } after { in map";
  constexpr static char INVALID_KEY[] = "Expected \" in map key";
  constexpr static char EXPECTED_COLON[] = "Expected ':' token after key in map";
  constexpr static char EXPECTED_COMMA[] = "Expected , after value in map";
  
  ++iterator; // iterator is {
  
  if (!findFirstNonWhitespace(iterator, end))
    throw ParsingException(UNFINISHED_MAP);
  
  auto *map = new JsonMap();
  JsonObject = new JsonObject(map);
  
  if (!findFirstNonWhitespace(iterator, end))
    throw ParsingException(UNFINISHED_MAP);
  
  for (; iterator != end; ++iterator) {
    if (*iterator == '}') return;
    
    if (*iterator != '\"')
      throw ParsingException(INVALID_KEY);
    
    std::string key;
    bool backslash = false;
    
    for (; iterator != end; ++iterator) {
      if (backslash) {
        backslash = false;
        key.push_back(*iterator);
      }
      
      switch (*iterator) {
        case '\"':
          backslash = true;
          break;
        case '\\':
          backslash = true;
          key.push_back('\\');
      }
    }
    
    if (!findFirstNonWhitespace(iterator, end) || *iterator != ':')
      throw ParsingException(EXPECTED_COLON);
    
    JsonObject *value{};
    interpretValue(iterator, end, value);
    
    if (!findFirstNonWhitespace(iterator, end))
      
      if (!findFirstNonWhitespace(iterator, end) || *iterator != ',') {
        if (*iterator)
          throw ParsingException(EXPECTED_COMMA);
      }
  }
}
