
#include <cstdarg>
#include <vector>
#include <iostream>
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
  prop(token_type, type);
  prop(char*, data);

protected:
  char *_description;

public:
  token() : token(token_type::unknown, NULL) {}
  token(token_type t, char *d) : _type(t), _data(d), _description(NULL) {}
  virtual ~token() { if (_data) free(_data); if (_description) free(_description); }
  virtual bool full() = 0;
  virtual char* description() { return _description; }
};

class sequence_token : public token {
  propro(size_t, length);
public:
  sequence_token(size_t len) : token(token_type::sequence, NULL), _length(len) {
    _data = (char *)calloc(len+1, sizeof(char));
  }
  bool full() {
    return strlen(_data) == _length;
  }
  char *description() {
    if (_description) free(_description);
    size_t len = _length;
    bool elip = false;
    if (len > 10) {
      len = 10;
      elip = true;
    }
    std::cout << "Creating desc w/ len " << len << std::endl;
    _description = (char *)calloc(5 + len + (elip ? 3 : 0), sizeof(char));
    memcpy(_description, "('", 2);
    memcpy(&_description[2], _data, len);
    if (!elip) {
      memcpy(&_description[2+len], "')", 3);
    } else {
      memcpy(&_description[2+len], "'...)", 6);
    }
    return _description;
  }
};

class tokenizer {
  propro(token*, tok);
public:
  tokenizer() : _tok(NULL) {}
  virtual ~tokenizer() {}
  virtual token* reset() = 0;
  virtual bool add_character(char c) = 0;
  virtual bool full() = 0;
};

class sequence_tokenizer : public tokenizer {
  propro(const char*, allowed_characters);
  propro(size_t, allowed_length);
public:
  sequence_tokenizer(const char *allowed, size_t len) : _allowed_characters(allowed), _allowed_length(len) {
    _tok = new sequence_token(len);
  }
  ~sequence_tokenizer() {
    delete dynamic_cast<sequence_token*>(_tok);
  }
  token* reset() {
    token *tmp = _tok;
    _tok = new sequence_token(_allowed_length);
    return tmp;
  }
  bool add_character(char c) {
    if (strchr(_allowed_characters, c) == NULL) {
      return false;
    }
    sequence_token *seqtok = dynamic_cast<sequence_token*>(_tok);
    if (seqtok->full()) {
      return false;
    }
    seqtok->data()[strlen(seqtok->data())] = c; // ehhh but the toker needs to manage the toks not toks themselves
    return true;
  }
  bool full() {
    sequence_token *seqtok = dynamic_cast<sequence_token*>(_tok);
    return seqtok->full();
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

using token_list = std::vector<token*>;
using tokenizer_list = std::vector<tokenizer*>;

class lexer {
  propro(token_list, tokens);
  propro(tokenizer_list, tokenizers);
  propro(char, current_character);
  propro(size_t, current_index);
  propro(bool, finished);
public:
  lexer() : _current_character(0), _current_index(0) {}
  lexer(tokenizer *t) : lexer() {
    add_tokenizer(t);
  }
  lexer(tokenizer *t, ...) {
    va_list args;
    va_start(args, t);
    vadd_tokenizers(t, args);
    va_end(args);
  }
  lexer add_tokenizer(tokenizer *t) {
    _tokenizers.push_back(t);
    return (*this);
  }
  lexer add_tokenizers(tokenizer *t, ...) {
    va_list args;
    va_start(args, t);
    vadd_tokenizers(t, args);
    va_end(args);
    return (*this);
  }
  lexer vadd_tokenizers(tokenizer *t, va_list args) {
    while (t != NULL) {
      _tokenizers.push_back(t);
      t = va_arg(args, tokenizer*);
    }
    return (*this);
  }
  lexer lex(const char *data) {
    std::vector<size_t> used;
    size_t ntokers = _tokenizers.size();
    used.reserve(ntokers);
    size_t i = 0, t = 0;
    while (i < strlen(data)) {
      _current_character = data[i];
      tokenizer *toker = _tokenizers[t];
      if (toker->add_character(_current_character)) {
        i++;
        used.clear();
      } else {
        if (toker->full()) {
          token *tok = toker->reset();
          _tokens.push_back(tok);
        } else {
          used.push_back(t);
        }
        if (used.size() != ntokers) {
          while (std::find(used.begin(), used.end(), t) != used.end()) {
            t = (t + 1) % ntokers;
          }
        } else {
          break;
        }
      }
    }
    tokenizer *toker = _tokenizers[t];
    token *tok = toker->reset();
    _tokens.push_back(tok);
    _current_index = i;
    _finished = (_current_index == strlen(data));
    return (*this);
  }
  lexer reset() {
    _tokens.clear();
    _finished = false;
    _current_character = 0;
    _current_index = 0;
    return (*this);
  }
};

#endif

