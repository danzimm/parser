
#include "properties.h"
#include "lexer.hpp"

#ifndef __grammar_H
#define __grammar_H

class grammar;

class grammar_component {
public:
  using token_id_list = std::vector<token::token_id>;

  propro(token::token_id, id);
  propro(token_id_list, allowed_before);
  propro(token_id_list, allowed_after);
  prop(grammar*, gram);
public:
  grammar_component(token::token_id idd) : _id(idd) {}
  virtual ~grammar_component() {}

  grammar_component& add_ids_before(token::token_id id, ...) {
    va_list args;
    va_start(args, id);
    _add_ids(&_allowed_before, id, args);
    va_end(args);
    return (*this);
  }

  grammar_component& add_ids_after(token::token_id id, ...) {
    va_list args;
    va_start(args, id);
    _add_ids(&_allowed_after, id, args);
    va_end(args);
    return (*this);
  }
  
  grammar_component& _add_ids(token_id_list *list, token::token_id id, va_list args) {
    while (id != 0) {
      list->push_back(id);
      id = va_arg(args, token::token_id);
    }
    return (*this);
  }

  virtual bool check_token_sequence(token* tok, token::token_id before, token::token_id after) {
    return (before == 0 || std::find(_allowed_before.begin(), _allowed_before.end(), before) != _allowed_before.begin()) && (after == 0 || std::find(_allowed_after.begin(), _allowed_after.end(), after) != _allowed_after.end()); 
  }
};

class grammar {
public:

  using grammar_component_map = std::map<token::token_id, grammar_component*>;

  propro(grammar_component_map, components);

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

  virtual bool check_token_sequence(token *before, token *itself, token *after) {
    token::token_id beforeid, afterid, meid;

    beforeid = before ? before->id() : 0;
    afterid = after ? after->id() : 0;
    meid = itself->id();

    return _check_token_sequence(beforeid, meid, afterid, before, itself, after);
  }

  virtual bool check_token_list(token_list& tokens) {
    size_t i, len = tokens.size();
    token *before, *after, *itself;
    for (i = 0; i < len; i++) {
      if (i == 0) {
        before = NULL;
      } else {
        before = tokens[i-1];
      }
      if (i == len - 1) {
        after = NULL;
      } else {
        after = tokens[i+1];
      }
      itself = tokens[i];
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
  bool _check_token_sequence(token::token_id beforeid, token::token_id meid, token::token_id afterid, token *before, token *itself, token *after) {
    grammar_component *comp;
    if ((comp = component_for_id(meid)) != NULL) {
      if (!comp->check_token_sequence(itself, beforeid, afterid)) {
        std::cerr << "Grammar error at token sequence: " << (before ? before->description() : "NULL") << " " << (itself ? itself->description() : "NULL") << " " << (after ? after->description() : "NULL") << std::endl;
        return false;
      }
    }
    return true;
  }

};

#endif

