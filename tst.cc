
#include <iostream>
#include <cstdint>
#include "lexer.hpp"

int main(int argc, const char *argv[]) {
  const char *dats;
  if (argc > 1) {
    dats = argv[1];
  } else {
    dats = "hello world";
  }

  /*
  auto keytoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    auto *toker = new alphai_tokenizer(8);
    toker->min_length(1);
    return toker;
  });
  auto separtoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return new sequence_tokenizer(":", 1);
  });
  auto istrtoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return new istr_tokenizer();
  });
  auto strtoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    return new container_tokenizer('"', '"', istrtoker, NULL);
  });
  auto objtoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    return new container_tokenizer('{', '}', keytoker, ntoker, separtoker, wstoker, strtoker, id, NULL);
  });
  */
  
  auto ntoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return new number_tokenizer();
  });
  auto wstoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    auto *toker = new whitespace_tokenizer(SIZE_MAX);
    toker->min_length(1);
    toker->type_description("whitespace");
    return toker;
  });
  auto hextoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    auto toker = new sequence_tokenizer("abcdefABCDEF0123456789", 6);
    toker->type_description("hex");
    return toker;
  });
  auto arrsepartoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    auto toker = new sequence_tokenizer(",", 1);
    toker->type_description("comma");
    return toker;
  });
  auto tupletoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    auto toker = new container_tokenizer('(',')', hextoker, ntoker, arrsepartoker, wstoker, NULL);
    toker->type_description("tuple");
    return toker;
  });
  auto arrtoker = tokenizer_table::register_tokenizer_creator([=] (tokenizer_id id) {
    auto toker = new container_tokenizer('[', ']', tupletoker, arrsepartoker, wstoker, NULL);
    toker->type_description("array");
    return toker;
  });
  
  lexer *ler = new lexer(arrtoker, wstoker, NULL);
  ler->lex(dats);
  auto toks = ler->tokens();
  if (!ler->finished()) {
    std::cout << "Lexer error at index `" << ler->current_index() << "`; character `" << ler->current_character() << "`" << std::endl;
  }
  for (auto tok : toks) {
    std::cout << tok->description() << " ";
  }
  std::cout << std::endl;
  return 0;
}

