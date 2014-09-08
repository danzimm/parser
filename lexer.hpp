
#include <cstdarg>
#include <vector>
#include <iostream>
#include <functional>
#include <map>
#include "properties.h"

#ifndef __lexer_H
#define __lexer_H

enum token_type {
  sequence,
  container,
  unknown,
  custom
};

class token {
public:
  inline char char_to_hex(char c) {
    switch (c) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
        return '0' + c;
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        return 'a' + (c - 10);
      default:
        return '0';
    }
  }

  inline void find_nicer_rep(char c, char out[4]) {
    out[0] = 0;
    switch(c) {
      case '\t':
        out[0] = '\\';
        out[1] = 't';
        out[2] = '\0';
        break;
      case '\n':
        out[0] = '\\';
        out[1] = 'n';
        out[2] = '\0';
        break;
      case '\r':
        out[0] = '\\';
        out[1] = 'r';
        out[2] = '\0';
        break;
      case 0x7f:
        out[0] = 'd';
        out[1] = 'e';
        out[2] = 'l';
        out[3] = '\0';
        break;
    }
  }

protected:
  char *_description;
  prop(token_type, type);
  prop(const char*, type_description);
  props(char*, data, {
    if (_data) {
      free(_data);
    }
  });

public:
  token() : token(token_type::unknown) {}
  token(token_type t) : _description(NULL), _type(t), _type_description("token") {
    _data = (char *)malloc(1*sizeof(char));
    _data[0] = '\0';
  }
  virtual ~token() {
    if (_data) {
      free(_data);
    }
    if (_description) free(_description);
  }
  virtual char *description() {
    if (_description) free(_description);
    size_t len = strlen(_data), i, j;
    bool elip = false;
    if (len > 10) {
      len = 10;
      elip = true;
    }
    _description = (char *)calloc(3,sizeof(char));
    memcpy(_description, "('", 3);
    for (i = 0; i < len; i++) {
      if (_data[i] >= ' ' && _data[i] <= '~') {
        _description = (char *)reallocf(_description, strlen(_description) + 2);
        _description[strlen(_description)+1] = '\0';
        _description[strlen(_description)] = _data[i];
      } else {
        bool restnull = true;
        for (j = i; j < len; j++) {
          if (_data[j] != '\0') {
            restnull = false;
            break;
          }
        }
        if (restnull) {
          break;
        }
        char nicer[4];
        find_nicer_rep(_data[i], nicer);
        if (nicer[0] != 0) {
          _description = (char *)reallocf(_description, strlen(_description) + strlen(nicer) + 1);
          strcpy(&_description[strlen(_description)], nicer);
        } else {
          _description = (char *)reallocf(_description, strlen(_description) + 5);
          char part[] = { '\\', 'x', char_to_hex(((_data[i] & 0xf0) >> 4)), char_to_hex((_data[i] & 0xf)), '\0' };
          strcpy(&_description[strlen(_description)], part);
        }
      }
    }
    if (!elip) {
      _description = (char *)reallocf(_description, strlen(_description) + 3);
      memcpy(&_description[strlen(_description)], "',", 3);
    } else {
      _description = (char *)reallocf(_description, strlen(_description) + 6);
      memcpy(&_description[strlen(_description)], "'...,", 6);
    }
    _description = (char *)reallocf(_description, strlen(_description) + strlen(_type_description) + 2);
    strcpy(&_description[strlen(_description)], _type_description);
    _description[strlen(_description)+1] = '\0';
    _description[strlen(_description)] = ')';
    return _description;
  }
  auto& append(char c) {
    size_t len = strlen(_data);
    _data = (char *)reallocf(_data, len + 2);
    _data[len+1] = '\0';
    _data[len] = c;
    return (*this);
  }
};

class tokenizer {
  using sequence_validator = std::function<bool(token*)>;
  propro(token*, tok);
  props(const char*, type_description, {
    _tok->type_description(val);
  });
  prop(sequence_validator, validator);

public:
  tokenizer() : _tok(NULL), _type_description(NULL) {}
  virtual ~tokenizer() {}
  virtual token* create_token() = 0;
  virtual token* reset() {
    token *tmp = _tok;
    if (_validator) {
      if (!_validator(tmp)) {
        delete tmp;
        tmp = NULL;
      }
    }
    _tok = create_token();
    if (_type_description) {
      _tok->type_description(_type_description);
    }
    return tmp;
  }
  virtual bool add_character(char c) = 0;
  virtual bool full() = 0;
  virtual size_t lex(const char *data) = 0;
};

using token_list = std::vector<token*>;

using tokenizer_id = size_t;
using tokenizer_id_list = std::vector<tokenizer_id>;

namespace tokenizer_table {
  using tokenizer_creator = std::function<tokenizer*(tokenizer_id)>;
  std::map<tokenizer_id, tokenizer_creator> tokenizer_creator_map;
  tokenizer_id max_id = 0;
  bool is_tokenizer_creator_registered(tokenizer_id id) {
    return tokenizer_creator_map.find(id) != tokenizer_creator_map.end();
  }
  tokenizer* create_tokenizer(tokenizer_id id) {
    if (is_tokenizer_creator_registered(id)) {
      return tokenizer_creator_map[id](id);
    }
    std::cerr << "Warning: Attempted to create tokenizer with id `" << id << "'" << std::endl;
    return NULL;
  }
  tokenizer_id register_tokenizer_creator(tokenizer_id id, tokenizer_creator creator) {
    if (id == 0) {
      throw "Error: Not allowed to take tokenizer_id = 0";
    }
    if (id > max_id) {
      max_id = id;
    }
    tokenizer_creator_map[id] = creator;
    return id;
  }
  tokenizer_id register_tokenizer_creator(tokenizer_creator creator) {
    tokenizer_id id = max_id+1;
    register_tokenizer_creator(id, creator);
    return id;
  }
}

class strict_sequence_tokenizer : public tokenizer {
  propro(const char*, sequence);
protected:
  size_t _sequenceLen;
public:
  strict_sequence_tokenizer(const char *str) : tokenizer(), _sequence(str) {
    _sequenceLen = strlen(str);
    _tok = new token(token_type::sequence);
    _tok->type_description(str);
  }
  ~strict_sequence_tokenizer() {
    delete _tok;
  }
  token* create_token() {
    return new token(token_type::sequence);
  }
  bool add_character(char c) {
    char *dat = _tok->data();
    if (c == _sequence[strlen(dat)]) {
      _tok->append(c);
      return true;
    }
    return false;
  }
  bool full() {
    return strlen(_tok->data()) == _sequenceLen;
  }
  size_t lex(const char *data) {
    size_t i = 0;
    while (i+1 < _sequenceLen && i < strlen(data)) {
      if (!add_character(data[i])) {
        break;
      }
      i++;
    }
    return i;
  }
};

class sequence_tokenizer : public tokenizer {
  propro(const char*, allowed_characters);
  props(size_t, max_length, {
    char *tmp = _tok->data();
    size_t tlen = strlen(tmp);
    _tok->data(NULL); // TODO: this is causing a free on the pointer to tmp - need to fix, create copy first
    if (val < tlen) {
      tmp[val] = '\0';
      tmp = (char *)reallocf(tmp, val+1);
    }
    _tok->data(tmp);
  });
  prop(size_t, min_length);

public:
  sequence_tokenizer(const char *allowed, size_t len) : tokenizer(), _allowed_characters(allowed), _max_length(len), _min_length(len) {
    _tok = new token(token_type::sequence);
  }
  ~sequence_tokenizer() {
    delete _tok;
  }
  token* create_token() {
    return new token(token_type::sequence);
  }
  bool add_character(char c) {
    if (strchr(_allowed_characters, c) == NULL) {
      return false;
    }
    _tok->append(c);
    return true;
  }
  bool full() {
    size_t len = strlen(_tok->data());
    return len >= _min_length && len <= _max_length;
  }
  size_t lex(const char *data) {
    size_t i = 0;
    while (i+1 < _max_length && i < strlen(data)) {
      if (!add_character(data[i])) {
        break;
      }
      i++;
    }
    return i;
  }
};

class istr_tokenizer : public tokenizer {
public:
  istr_tokenizer() : tokenizer() {
    _tok = new token(token_type::sequence);
  }
  ~istr_tokenizer() {
    delete _tok;
  }
  token* create_token() {
    return new token(token_type::sequence);
  }
  bool add_character(char c) {
    if (c < ' ' || c > '~') {
      return false;
    }
    if (c == '"') {
      if (!was_escape()) {
        return false;
      }
    }
    _tok->append(c);
    return true;
  }
  bool was_escape() {
    char *d = _tok->data();
    size_t len = strlen(d);
    return len > 0 && d[len-1] == '\\';
  }
  bool full() {
    return strlen(_tok->data()) > 0;
  }
  size_t lex(const char *data) {
    size_t i = 0;
    while (i < strlen(data)) {
      if (!add_character(data[i])) {
        break;
      }
      i++;
    }
    return i;
  }
};

#define seqtoker(m, c) \
  class m ## _tokenizer : public sequence_tokenizer { \
  public: \
    m ## _tokenizer (size_t len) : sequence_tokenizer(c, len) {} \
  };

seqtoker(lalpha, "abcdefghijklmnopqrstuvwxyz")
seqtoker(ualpha, "ABCDEFGHIJKLMNOPQRSTUVWXYZ")
seqtoker(alphai, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
seqtoker(numeric, "1234567890")
seqtoker(lalphanumeric, "abcdefghijklmnopqrstuvwxyz1234567890")
seqtoker(ualphanumeric, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890")
seqtoker(alphainumeric, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890")
seqtoker(space, " ")
seqtoker(whitespace, " \t\n\r")
seqtoker(ascii, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");

class number_tokenizer : public tokenizer {
  propro(bool, negative);
  propro(bool, has_decimal);
  propro(bool, exponential);

public:
  number_tokenizer() : tokenizer(), _negative(false), _has_decimal(false), _exponential(false) {
    _tok = new token(token_type::sequence);
    type_description("number");
  }
  ~number_tokenizer() {
    delete _tok;
  }
  token* create_token() {
    return new token(token_type::sequence);
  }
  bool add_character(char c) {
    size_t len = strlen(_tok->data());
    if (len == 0 || was_exponent()) {
      if (c == '-') {
        _negative = true;
        goto appendit;
      }
    }
    if ((len > 1 && !_negative) || (len > 2 && _negative)) {
      if (c == 'e') {
        _exponential = true;
        _has_decimal = false;
        goto appendit;
      }
    }
    if (!_has_decimal) {
      if (c == '.') {
        _has_decimal = true;
        goto appendit;
      }
    }
    if (c >= '0' && c <= '9') {
      goto appendit;
    }
    return false;
appendit:
    _tok->append(c);
    return true;
  }
  bool was_exponent() {
    char *d = _tok->data();
    size_t len = strlen(d);
    return len > 0 && d[len-1] == 'e';
  }
  bool full() {
    size_t len = strlen(_tok->data());
    return ((len > 0 && !_negative) || (len > 1 && _negative)) && !was_exponent();
  }
  size_t lex(const char *data) {
    size_t i = 0;
    while (i < strlen(data)) {
      if (!add_character(data[i])) {
        break;
      }
      i++;
    }
    return i;
  }
};

class lexer {
  propro(token_list, tokens);
  propro(tokenizer_id_list, tokenizers);
  propro(char, current_character);
  propro(size_t, current_index);
  propro(bool, finished);
  prop(bool, verbose);
public:
  lexer() : _current_character(0), _current_index(0), _verbose(false) {}
  lexer(tokenizer_id t, ...) : lexer() {
    va_list args;
    va_start(args, t);
    add_tokenizers(t, args);
    va_end(args);
  }
  lexer(tokenizer_id_list list) : lexer() {
    add_tokenizers(list);
  }
  lexer& add_tokenizer(tokenizer_id t) {
    _tokenizers.push_back(t);
    return (*this);
  }
  lexer& add_tokenizers(tokenizer_id_list list) {
    for (auto id : list) {
      _tokenizers.push_back(id);
    }
    return (*this);
  }
  lexer& add_tokenizers(tokenizer_id id, ...) {
    va_list args;
    va_start(args, id);
    add_tokenizers(id, args);
    va_end(args);
    return (*this);
  }
  lexer& add_tokenizers(tokenizer_id t, va_list args) {
    while (t != 0) {
      add_tokenizer(t);
      t = va_arg(args, tokenizer_id);
    }
    return (*this);
  }
  lexer& lex(const char *data) {
    std::vector<tokenizer*> tokers;
    for (auto id : _tokenizers) {
      tokers.push_back(tokenizer_table::create_tokenizer(id));
    }
    std::vector<size_t> used;
    size_t ntokers = tokers.size();
    used.reserve(ntokers);
    size_t i = 0, t = 0;
    while (i < strlen(data)) {
      _current_character = data[i];
      tokenizer *toker = tokers[t];
      if (toker->add_character(_current_character)) {
        if (_verbose) {
          std::cout << "Tokenizer " << _tokenizers[t] << " eating `" << _current_character << "`" << std::endl;
        }
        size_t ii = toker->lex(&data[++i]);
        if (toker->full()) {
          i += ii;
          token *tok = toker->reset();
          _tokens.push_back(tok);
          used.clear();
          t = 0;
          continue;
        } else {
          token *tok = toker->reset();
          delete tok;
          i--;
        }
      }
      used.push_back(t);
      if (used.size() != ntokers) {
        while (std::find(used.begin(), used.end(), t) != used.end()) {
          t = (t + 1) % ntokers;
        }
      } else {
        break;
      }
    }
    tokenizer *toker = tokers[t];
    if (toker->full()) {
      token *tok = toker->reset();
      _tokens.push_back(tok);
    }
    _current_index = i;
    _finished = (_current_index == strlen(data));
    for (i = 0; i < ntokers; i++) {
      delete tokers[i];
    }
    tokers.clear();
    return (*this);
  }
  lexer& reset() {
    _tokens.clear();
    _finished = false;
    _current_character = 0;
    _current_index = 0;
    return (*this);
  }
};

class container_token : public token {
  propro(token_list, tokens);
  propro(char, begin);
  propro(char, end);

public:
  container_token() : token(token_type::container) {
    type_description("container");
    free(_data);
    _data = NULL;
  }
  container_token& boundary(char b, char e) {
    _begin = b;
    _end = e;
    return (*this);
  }
  container_token& addTokens(token_list toks) {
    for (auto tok : toks) {
      _tokens.push_back(tok);
    }
    return (*this);
  }
  char *description() {
    if (_description) free(_description);
    char tmp[] = { '{', '\'', _begin, '\'', ',', '\'', _end, '\'', ',', '\0' };
    _description = (char *)calloc(strlen(tmp)+1, sizeof(char));
    strcpy(_description, tmp);
    for (auto tok : _tokens) {
      char *desc = tok->description();
      _description = (char *)reallocf(_description, strlen(_description) + strlen(desc) + 2);
      strcat(_description, desc);
      size_t len = strlen(_description);
      _description[len+1] = '\0';
      _description[len] = ',';
    }
    _description = (char *)reallocf(_description, strlen(_description) + 2);
    size_t len = strlen(_description);
    _description[len-1] = '}';
    return _description;
  }
};

class container_tokenizer : public tokenizer {
  propro(char, begin);
  propro(char, end);
  propro(tokenizer_id_list, tokenizers);

protected:
  bool _full;

public:
  container_tokenizer(char begin, char end) : tokenizer(), _begin(begin), _end(end), _full(false) {
    _tok = new container_token();
    dynamic_cast<container_token*>(_tok)->boundary(begin, end);
  }
  container_tokenizer(char begin, char end, tokenizer_id_list list) : container_tokenizer(begin, end) {
    add_tokenizers(list);
  }
  container_tokenizer(char begin, char end, tokenizer_id t, ...) : container_tokenizer(begin, end) {
    va_list args;
    va_start(args, t);
    add_tokenizers(t, args);
    va_end(args);
  }
  ~container_tokenizer() {
    delete _tok;
  }
  bool full() {
    return _full;
  }
  token* create_token() {
    auto *t = new container_token();
    dynamic_cast<container_token*>(t)->boundary(_begin, _end);
    return t;
  }
  token* reset() {
    auto *retval = tokenizer::reset();
    _full = false;
    return retval;
  }
  container_tokenizer& add_tokenizer(tokenizer_id t) {
    _tokenizers.push_back(t);
    return (*this);
  }
  container_tokenizer& add_tokenizers(tokenizer_id_list list) {
    for (auto id : list) {
      add_tokenizer(id);
    }
    return (*this);
  }
  container_tokenizer& add_tokenizers(tokenizer_id t, ...) {
    va_list args;
    va_start(args, t);
    add_tokenizers(t, args);
    va_end(args);
    return (*this);
  }
  container_tokenizer& add_tokenizers(tokenizer_id t, va_list args) {
    while (t != 0) {
      add_tokenizer(t);
      t = va_arg(args, tokenizer_id);
    }
    return (*this);
  }
  bool add_character(char c) {
    if (c == _begin) {
      return true;
    }
    return false;
  }
  size_t lex(const char *data) { 
    lexer *lex = new lexer(_tokenizers);
    lex->lex(data);
    auto toks = lex->tokens();
    if (toks.size() == 0) {
    }
    if (!lex->finished()) {
      if (lex->current_character() == _end) {
        dynamic_cast<container_token*>(_tok)->addTokens(toks);
        _full = true;
        auto retval = lex->current_index();
        delete lex;
        return retval + 1;
      } else {
        std::cout << "Container failed to find ending character " << _end << std::endl;
      }
    }
    std::cout << "Container lexer (" << _begin << "," << _end << ") stopped: " << lex->current_index() << " '" << lex->current_character() << "' with start '" << data[0] << "'" << std::endl;
    for (auto tok : toks) {
      delete tok;
    }
    toks.clear();
    delete lex;
    return 0;
  }
};

#endif

