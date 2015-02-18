
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
    return (before == 0 || std::find(_allowed_before.begin(), _allowed_before.end(), before) != _allowed_before.end()) && (after == 0 || std::find(_allowed_after.begin(), _allowed_after.end(), after) != _allowed_after.end()); 
  }
};

class grammar {
public:

  using grammar_component_map = std::map<token::token_id, grammar_component*>;
  using token_id_list = grammar_component::token_id_list;

  propro(grammar_component_map, components);
  propro(token_id_list, ignored_tokens);
  
// TODO: grammar comp should only allow before and after in pairs. i.e. shouldnt be able to have a keytoken just sittin alone, needs to either come after new line and before separ token or after separ and before new line, not before and after newline nor nefore and after separ. This is in general a major problem, unsure how to fix it. We shouldn't allow : to occur more than once per line as well. In general there needs to be some sort of better checking going on here...
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
      return false; // if for some reason you didn't create a grammar_component for this token then you should be shamed
    }
    return true;
  }

};

#endif

