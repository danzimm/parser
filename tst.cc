#include <iostream>
#include "lexer.hpp"

int main(int argc, const char *argv[]) {
  const char *dats;
  if (argc > 1) {
    dats = argv[1];
  } else {
    dats = "hello world";
  }
  auto *alntoker = new alphai_tokenizer(1);
  auto *wstoker = new whitespace_tokenizer(1);
  lexer *ler = new lexer(alntoker, wstoker, NULL);
  ler->lex(dats);
  auto toks = ler->tokens();
  if (!ler->finished()) {
    std::cout << "Not finished! At index `" << ler->current_index() << "` and last char `" << ler->current_character() << "`" << std::endl;
  }
  for (auto tok : toks) {
    std::cout << tok->description() << " ";
  }
  std::cout << std::endl;
  return 0;
}
