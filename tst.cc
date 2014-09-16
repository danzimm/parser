
#include <iostream>
#include <cstdint>
#include "lexer.hpp"
#include "grammar.hpp"
#include "utils.hpp"

#include "line_grammar.hpp"

int main(int argc, const char *argv[]) {
  const char *dats;
  if (argc > 1) {
    dats = argv[1];
  } else {
    dats = "hello world";
  }
  
  /*
  auto ntoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(number_tokenizer, (), );
  });
  auto wstoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(whitespace_tokenizer, (SIZE_MAX), obj->min_length(1).type_description("whitespace"));
  });
  auto hextoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(sequence_tokenizer, ("abcdefABCDEF0123456789", 6), obj->type_description("hex color"));
  });
  auto arrsepartoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(sequence_tokenizer, (",", 1), obj->type_description("comma"));
  });
  auto keytoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(alphai_tokenizer, (8), obj->min_length(1));
  });
  auto separtoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(sequence_tokenizer, (":", 1), );
  });
  auto istrtoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(istr_tokenizer, (), );
  });
  auto strtoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    return create_object(container_tokenizer, ('"', '"', istrtoker, NULL), );
  });
  auto tupletoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    return create_object(container_tokenizer, ('(', ')', hextoker, ntoker, arrsepartoker, wstoker, NULL), obj->type_description("tuple"));
  });
  auto arrtoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    return create_object(container_tokenizer, ('[',']', tupletoker, arrsepartoker, strtoker, wstoker, NULL), obj->type_description("array"));
  });
  auto objtoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    return create_object(container_tokenizer, ('{','}', separtoker, arrsepartoker, keytoker, ntoker, strtoker, id, arrtoker, wstoker, NULL), );
  });

  lexer *ler = new lexer(keytoker, ntoker, strtoker, separtoker, objtoker, arrtoker, tupletoker, wstoker, NULL);
  */

  auto ntoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(number_tokenizer, (), );
  });
  auto wstoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(sequence_tokenizer, (" \t", SIZE_MAX), obj->min_length(1).type_description("whitespace"));
  });
  auto nltoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(sequence_tokenizer, ("\r\n", SIZE_MAX), obj->min_length(1).type_description("newline"));
  });
  auto separtoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(sequence_tokenizer, (":", 1), );
  });
  auto keytoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(alphai_tokenizer, (8), obj->min_length(1));
  });
  lexer *ler = new lexer(keytoker, ntoker, separtoker, wstoker, nltoker, NULL);

  ler->lex(dats);
  auto toks = ler->tokens();
  if (!ler->finished()) {
    std::cout << "Lexer error at index `" << ler->current_index() << "`; character `" << ler->current_character() << "`" << std::endl;
  }
  for (auto tok : toks) {
    std::cout << tok->description() << " ";
  }
  std::cout << std::endl;
  
  auto* sepcomp = new line_grammar_component(separtoker);
  sepcomp->validator([=](line_grammar_component& self, grammar_component::token_id_list& toks) {
    auto begin = toks.begin();
    auto end = toks.end();
    auto id = self.id();
    if (std::find(begin, end, id) != end) {
      std::cerr << "Grammar error found a second : in the same line" << std::endl;
      return false;
    }
    auto nkey = std::count(begin, end, keytoker);
    if (nkey != 1) {
      std::cerr << "Grammar error found the wrong number of keys before the `:': " << nkey << std::endl;
      return false;
    }
    return true;
  });
  sepcomp->add_token_before(keytoker, 0);
  auto* nlcomp = new line_grammar_component(nltoker);
  nlcomp->validator([=](line_grammar_component& self, grammar_component::token_id_list& toks) {
    auto begin = toks.begin();
    auto end = toks.end();
    auto loc = std::find(begin, end, separtoker);
    if (loc != end) {
      if (std::find(loc, end, keytoker) == end && std::find(loc, end, ntoker) == end) {
        std::cerr << "Grammar error failed to find value for a key" << std::endl;
        return false;
      }
    }
    return true;
  });
  line_grammar *gram = new line_grammar(sepcomp, nlcomp, NULL);
  gram->newlineid(nltoker);
  if (gram->check_token_list(toks)) {
    std::cout << "Passed grammar check" << std::endl;
  }
  
  return 0;
}

