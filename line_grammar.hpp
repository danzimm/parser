
#include "grammar.hpp"

#ifndef __line_grammar_H
#define __line_grammar_H

class line_grammar : public grammar {
  prop(token::token_id, newlineid);
  propro(bool, afternewline);
  propro(grammar_component::token_id_list, line_tokens);

public:
  line_grammar(grammar_component *comp, ...) : _newlineid(SIZE_MAX), _afternewline(false) {
    va_list args;
    va_start(args, comp);
    _add_components(comp, args);
    va_end(args);
  }

  virtual bool check_token_sequence(token *before, token *itself, token *after) {
    token::token_id beforeid, afterid, meid;

    beforeid = before ? before->id() : 0;
    afterid = after ? after->id() : 0;
    meid = itself ? itself->id() : 0;
    
    bool succ = _check_token_sequence(beforeid, meid, afterid, before, itself, after);
    
    if (succ) {
      if (meid == _newlineid) {
        _afternewline = true; 
        _line_tokens.clear();
      } else {
        _line_tokens.push_back(meid);
        _afternewline = false;
      }
      return true;
    }
    return false;
  }

  virtual bool check_token_list(token_list& tokens) {
    bool retval = grammar::check_token_list(tokens);
    if (retval) {
      auto len = tokens.size();
      token *before = len > 0 ? tokens[len-1] : NULL;
      retval = _check_token_sequence(before->id(), _newlineid, 0, before, NULL, NULL);
    }
    return retval;
  }
};

class line_grammar_component : public grammar_component {
  using line_validator = std::function<bool(line_grammar_component&, token_id_list&)>;
  propro(token_id_list, required_before);
  prop(line_validator, validator);

public:
  line_grammar_component(token::token_id idd) : grammar_component(idd), _validator(NULL) {}
  
  line_grammar_component& add_token_before(token::token_id id, ...) {
    va_list args;
    va_start(args, id);
    _add_token_before(id, args);
    va_end(args);
    return (*this);
  }

  line_grammar_component& _add_token_before(token::token_id id, va_list args) {
    while (id != 0) {
      _required_before.push_back(id);
      id = va_arg(args, token::token_id);
    }
    return (*this);
  }

  virtual bool check_token_sequence(token* tok, token::token_id before, token::token_id after) {
    line_grammar *gramm = dynamic_cast<line_grammar*>(_gram);
    auto linetoks = gramm->line_tokens();
    if (_validator) {
      return _validator((*this), linetoks);
    }
    auto end = linetoks.end();
    auto begin = linetoks.begin();
    for (auto required : _required_before) {
      if (std::find(begin, end, required) == end) {
        return false;
      }
    }
    return !gramm->afternewline();
  }

};

#endif

