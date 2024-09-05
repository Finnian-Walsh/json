//
// Created by finni on 23/06/2024.
//

#include "ChunkedList.hpp"

#include <functional>
#include <string>
#include <exception>
#include <cmath>
#include <cstdint>
#include <map>
#include <csetjmp>
#include <variant>

namespace Json {
  class JsonObject;
  
  using const_string_it = std::string::const_iterator;
  
  enum JsonType {
    null,
    boolean,
    number,
    string,
    list,
    map,
    undefined,
  };
  
  class JsonNull {};
  
  class JsonBoolean {
    private:
      bool value{};
    public:
      inline explicit JsonBoolean(bool state);
      
      inline operator bool() const;
      
      inline JsonBoolean &operator=(bool newValue);
  };
  
  class JsonNumber {
    private:
      class BadNumberException : public std::exception {
        private:
          std::string message = "Unable to convert string to number";
        public:
          inline BadNumberException();
          
          inline explicit BadNumberException(std::string &&str);
          
          ~BadNumberException() override;
          
          [[nodiscard]] const char *what() const noexcept override;
      };
      
      std::string str;
    public:
      explicit JsonNumber(std::string &&str);
      
      using Exception = BadNumberException;
      
      constexpr void assert();
      
      template<typename DecimalT = double>
      DecimalT asDecimal();
      
      template<typename IntT = int>
      IntT asInteger();
      
      const std::string &asString();
      
      inline JsonNumber &operator=(std::string &&newStr) {
        str = std::move(newStr);
        return *this;
      }
      
      inline JsonNumber &operator=(std::string &newStr) {
        str = std::move(newStr);
        return *this;
      }
  };
  
  class JsonString : public std::string {
    public:
      inline explicit JsonString(std::string &&);
      
      inline explicit JsonString(std::string &);
      
      inline explicit JsonString(char *);
      
      [[maybe_unused]] inline std::string &basicString();
      
      [[maybe_unused]] inline void lower();
      
      [[maybe_unused]] inline void upper();
  };
  
  class JsonList : public ChunkedList<JsonObject, 32> {};
  
  class JsonMap : public std::unordered_map<JsonString, JsonObject> {};
  
  class JsonObject {
    public:
      using Null = JsonNull;
      using Boolean = JsonBoolean;
      using Number = JsonNumber;
      using String = JsonString;
      using List = JsonList;
      using Map = JsonMap;
    private:
      using JsonVariant = std::variant<Null, Boolean, Number, String, List, Map>;
      JsonVariant value{};
    public:
      JsonObject();
      
      template<typename T>
      explicit JsonObject(T data);
      
      ~JsonObject() = default;
      
      template<typename T>
      [[maybe_unused]] T *as();
      
      template<typename T>
      JsonObject &operator=(T data);
  };
  
  namespace Parser {
    class ParsingException : public std::exception {
      private:
        std::string message = "JSON parsing error; unable to parse JSON";
      public:
        ParsingException();
        
        explicit ParsingException(const std::string &information);
        
        [[nodiscard]] const char *what() const noexcept override;
    };
    
    using Exception = ParsingException;
    
    void interpretValue(const_string_it &, const_string_it, JsonObject &);
    
    void parse(JsonObject &JsonObject, const std::string &jsonString);
    
    bool iteratorAligns(const_string_it &iterator, const_string_it end, const std::string &s);
    
    inline bool isWhitespaceOrComma(char c);
    
    inline bool isWhitespace(char c);
    
    char findFirstNonWhitespace(const_string_it &iterator, const_string_it end);
    
    void parseNull(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
    
    void parseTrue(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
    
    void parseFalse(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
    
    void parseNumber(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
    
    void parseString(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
    
    void parseList(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
    
    void parseMap(const_string_it &iterator, const_string_it end, JsonObject &JsonObject);
  }
}
