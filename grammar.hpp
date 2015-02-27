
#include "properties.h"
#include "lexer.hpp"
#include <tuple>

#ifndef __grammar_H
#define __grammar_H

class grammar;

class grammar_component {
public:
  using token_id = token::token_id;
  using token_tuple = std::tuple<token_id, token_id>;
  using token_tuple_list = std::vector<token_tuple>;

  propro(token_id, id);
  propro(token_tuple_list, allowed_around);
  prop(grammar*, gram);
public:
  grammar_component(token_id idd) : _id(idd) {}
  virtual ~grammar_component() {}

  grammar_component& add_ids_around(token_id ida, token_id idb) {
    _allowed_around.push_back(token_tuple(ida, idb));
    return (*this);
  }
  grammar_component& add_ids_around(token_tuple tup) {
    _allowed_around.push_back(tup);
    return (*this);
  }
  grammar_component& add_ids_combination(std::vector<token_id> befores, std::vector<token_id> afters) {
    for (auto btokid : befores) {
      for (auto atokid : afters) {
        add_ids_around(btokid, atokid);
      }
    }
    return (*this);
  }

  virtual bool check_token_sequence(token* tok, token_id before, token_id after) {
    bool allowed = false;
    for (token_tuple tup : _allowed_around) {
      token_id ida = std::get<0>(tup);
      token_id idb = std::get<1>(tup);
      if (ida == before && idb == after) {
        allowed = true;
        break;
      }
    }
    return allowed;
  }
};

class grammar {
public:
  
  using token_id = token::token_id;
  using grammar_component_map = std::map<token::token_id, grammar_component*>;
  using token_id_list = std::vector<token_id>;

  propro(grammar_component_map, components);
  propro(token_id_list, ignored_tokens);
  
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
    token::token_id id;
    while (comp != NULL) {
      id = comp->id();
      if (component_for_id(id) != NULL) {
        std::cerr << "Failed to add grammar component for id " << id << " since one already exists for this id!" << std::endl;
        delete comp;
      } else {
        comp->gram(this);
        _components[id] = comp;
      }
      comp = va_arg(args, grammar_component*);
    }
    return (*this);
  }

  grammar_component* component_for_id(token::token_id id) {
    if (_components.find(id) != _components.end()) {
      return _components[id];
    }
    return NULL;
  }

  virtual bool check_token_list(token_list& tokens) {
    size_t i, len = tokens.size();
    token *before, *after, *itself;
    for (i = 0; i < len; i++) {
      size_t index = i;
      itself = next_non_ignored_token(tokens, &index, true);
      i = index;
      before = i != 0 ? next_non_ignored_token(tokens, i - 1, false) : NULL;
      after = next_non_ignored_token(tokens, i + 1, true);
      if (!check_token_sequence(before, itself, after)) {
        break;
      }
    }
    if (i != len) {
      std::cerr << "Grammar error at token number " << i << ": " << tokens[i]->description() << std::endl;
      return false;
    }
    return true;
  }

protected:
  virtual token* next_non_ignored_token(token_list& tokens, size_t ii, bool forward) {
    size_t i = ii;
    return next_non_ignored_token(tokens, &i, forward);
  }
  virtual token* next_non_ignored_token(token_list& tokens, size_t *ii, bool forward) {
    size_t len = tokens.size(), i = *ii;
    for (; i < len; forward ? i++ : i--) {
      token* tok = tokens[i];
      token::token_id id = tok->id();
      if (std::find(_ignored_tokens.begin(), _ignored_tokens.end(), id) == _ignored_tokens.end()) {
        *ii = i;
        return tok;
      }
      if (i == 0)
        break;
    }
    *ii = i;
    return NULL;
  }
  virtual bool check_token_sequence(token *before, token *itself, token *after) {
    token::token_id beforeid, afterid, meid;

    beforeid = before ? before->id() : 0;
    afterid = after ? after->id() : 0;
    meid = itself->id();

    return _check_token_sequence(beforeid, meid, afterid, before, itself, after);
  }

  bool _check_token_sequence(token::token_id beforeid, token::token_id meid, token::token_id afterid, token *before, token *itself, token *after) {
    grammar_component *comp;
    if ((comp = component_for_id(meid)) != NULL) {
      if (!comp->check_token_sequence(itself, beforeid, afterid)) {
        // TODO: make verbose a property
        std::cerr << "Grammar error at token sequence: " << (before ? before->description() : "NULL") << " " << (itself ? itself->description() : "NULL") << " " << (after ? after->description() : "NULL") << std::endl;
        return false;
      }
    } else {
      std::cerr << "Failed to find a component for " << itself->description() << std::endl;
      return false; // if for some reason you didn't create a grammar_component for this token then you should be shamed
    }
    return true;
  }

};

#endif

