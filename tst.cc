
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include "lexer.hpp"
#include "grammar.hpp"
#include "utils.hpp"
#include <cassert>

struct linked_dict {
  char *key;
  enum {
    str_type,
    num_type
  } type;
  union {
    char *str;
    double num;
  };
  linked_dict *next, *previous;
  linked_dict(char *k) : str(NULL) {
    key = (char *)calloc(strlen(k)+1, sizeof(char));
    strcpy(key, k);
  }
  ~linked_dict() {
    if (type == str_type && str != NULL)
      free(str);
    if (key != NULL)
      free(key);
  }
  linked_dict& set_val(char *s) {
    type = str_type;
    str = (char *)calloc(strlen(s)+1,sizeof(char));
    strcpy(str, s);
    return (*this);
  }
  linked_dict& set_val(double n) {
    type = num_type;
    num = n;
    return (*this);
  }
};

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
  auto comtoker = tokenizer_table::register_tokenizer_creator([] (tokenizer_id id) {
    return create_object(prefixed_tokenizer, ("#", [] (char c) { return c != '\r' && c != '\n'; }), );
  });
  lexer *ler = new lexer(keytoker, ntoker, separtoker, wstoker, nltoker, comtoker, NULL);
  ler->lex(dats);
  auto toks = ler->tokens();
  if (!ler->finished()) {
    std::cout << "Lexer error at index `" << ler->current_index() << "`; character `" << ler->current_character() << "`" << std::endl;
  }
  for (auto tok : toks) {
    std::cout << tok->description() << " ";
  }
  std::cout << std::endl;

  auto* component = new grammar_component();
  component->initializer([=] (grammar_component* comp, grammar_component::comp_meta_data& meta_data, grammar_component::token_id id) {
    meta_data["withinline"] = "n";
    meta_data["separated"] = "n";
    return id != ntoker && id != separtoker;
  }).validator([=] (grammar_component* comp, grammar_component::comp_meta_data& meta_data, grammar_component::token_id id) {
    if (id == wstoker)
      return true;
    if (meta_data["withinline"][0] == 'n') {
      if (id != nltoker) {
        meta_data["withinline"] = "y";
        return id == comtoker || id == keytoker;
      }
    } else {
      
    }
  });

  size_t i;
  linked_dict* previous = NULL;
  linked_dict* current = NULL;
  for (i = 0; i < toks.size(); i++) {
    auto tok = toks[i];
    auto id = tok->id();
    if (id == nltoker || id == comtoker || id == wstoker)
      continue;
    if (current == NULL) {
      assert(id == keytoker);
      current = new linked_dict(tok->data());
      if (previous)
        previous->next = current;
      current->previous = previous;
    } else {
      if (id == separtoker)
        continue;
      current->set_val(tok->data());
      previous = current;
      current = NULL;
    }
  }
  current = previous;
  while (current->previous) {
    current = current->previous;
  }
  while(current) {
    std::cout << current->key << " : " << current->str << std::endl;
    current = current->next;
  }
  return 0;
}

