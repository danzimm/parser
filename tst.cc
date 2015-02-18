
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include "lexer.hpp"
#include "grammar.hpp"
#include "utils.hpp"

#include "line_grammar.hpp"

char *load_file(const char *path) {
  int fd = open(path, O_RDONLY);
  char *buff = (char *)calloc(1, sizeof(char));
  buff[0] = '\0';
  size_t len = 0;
  char tmp[1024];
  bzero(tmp, 1024);
  while (true) {
    size_t s = read(fd, tmp, 1024);
    buff = (char *)reallocf(buff, 1 + len + s);
    memcpy(buff + len, tmp, s);
    buff[len + s] = '\0';
    len += s;
    if (s != 1024)
      break;
  }
  return buff;
}

int main(int argc, const char *argv[]) {
  const char *dats;
  switch (argc) {
    case 3:
      dats = load_file(argv[2]); // mem leak i know but this is tst program so ok
      break;
    case 2:
      dats = argv[1];
      break;
    case 1:
    default:
      dats = "hello world";
      break;
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
  
  auto* ncomp = new grammar_component(ntoker);
  ncomp->add_ids_before(separtoker, 0).add_ids_after(nltoker, 0);
  auto* nlcomp = new grammar_component(nltoker);
  nlcomp->add_ids_before(ntoker, nltoker, keytoker, 0).add_ids_after(nltoker, keytoker);
  auto* separcomp = new grammar_component(separtoker);
  separcomp->add_ids_before(keytoker, 0).add_ids_after(ntoker, keytoker, 0);
  auto* keycomp = new grammar_component(keytoker);
  keycomp->add_ids_before(nltoker, separtoker, 0).add_ids_after(separtoker, nltoker, 0);

  grammar *gram = new grammar(ncomp, nlcomp, separcomp, keycomp, 0);
  gram->add_ignored_tokens(wstoker, 0);
  if (gram->check_token_list(toks)) {
    std::cout << "Passed grammar check" << std::endl;
  }
  
  return 0;
}

