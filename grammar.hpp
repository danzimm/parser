
#include "properties.h"
#include "lexer.hpp"
#include <tuple>

#ifndef __grammar_H
#define __grammar_H

class grammar;

class grammar_component {
public:
  using token_id = token::token_id;
  using comp_meta_data = std::map<std::string, std::string>;
  using comp_validator = std::function<bool(grammar_component*, comp_meta_data&, token_id)>;
  using comp_meta_data_initializer = std::function<bool(grammar_component*, comp_meta_data&, token_id)>;

  prop(grammar*, gram);
  prop(comp_meta_data_initializer, initializer);
  prop(comp_validator, validator);
  propro(comp_meta_data, meta_data);
public:
  grammar_component() : _gram(NULL), _initializer(NULL), _validator(NULL), _meta_data(comp_meta_data()) {
    _meta_data["finished"] = "n";
  }
  virtual ~grammar_component() {
  }
  virtual bool can_add_token(token_id id) {
    return _initializer ? _initializer(this, _meta_data, id) : false;
  }
  virtual bool add_token(token_id id) {
    return _validator ? _validator(this, _meta_data, id) : false;
  }
  virtual bool full() {
    return _meta_data["finished"][0] == 'y';
  }
  virtual size_t parse(token_list& list, size_t i) {
    while (i < list.size()) {
      bool succ = add_token(list[i]->id());
      if (!succ) {
        break;
      }
      i++;
    }
    return i;
  }
};

class grammar {
public:
  using token_id = token::token_id;
  using grammar_component_list = std::vector<grammar_component*>;
  using token_id_list = std::vector<token_id>;

  propro(grammar_component_list, components);
  propro(token_id_list, ignored_tokens);
  propro(bool, finished);
  propro(size_t, current_index);
  
public:
  grammar() {} 
  grammar(grammar_component *comp, ...) {
    va_list args;
    va_start(args, comp);
    _add_components(comp, args);
    va_end(args);
  }
  virtual ~grammar() {
    size_t i, len = _components.size();
    for (i = 0; i < len; i++) {
      delete _components[i];
    }
    _components.clear();
  }
  grammar& add_components(grammar_component *comp, ...) {
    va_list args;
    va_start(args, comp);
    _add_components(comp, args);
    va_end(args);
    return (*this);
  }
  grammar& add_ignored_tokens(token::token_id id, ...) {
    va_list args;
    va_start(args, id);
    while (id != 0) {
      _ignored_tokens.push_back(id);
      id = va_arg(args, token::token_id);
    }
    return (*this);
  }
  grammar& _add_components(grammar_component *comp, va_list args) {
    while (comp != NULL) {
      comp->gram(this);
      _components.push_back(comp);
      comp = va_arg(args, grammar_component*);
    }
    return (*this);
  }

  virtual grammar& check_token_list(token_list& tokens) {
    std::vector<size_t> used;
    size_t ncomps = _components.size();
    used.reserve(ncomps);
    size_t i = 0, t = 0;
    while (i < tokens.size()) {
      token* tok = tokens[i];
      grammar_component* comp = _components[t];
      if (comp->can_add_token(tok->id())) {
        size_t ii = comp->parse(tokens, ++t);
        if (comp->full()) {
          i += ii;
          used.clear();
          t = 0;
          continue;
        } else {
          i--;
        }
      }
      used.push_back(t);
      if (used.size() != ncomps) {
        while (std::find(used.begin(), used.end(), t) != used.end()) {
          t = (t + 1) % ncomps;
        }
      } else {
        break;
      }
    }
    _current_index = i;
    _finished = (_current_index == tokens.size());
    return (*this);
  }
};

#endif

