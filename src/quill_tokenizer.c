#include "quill_tokenizer.h"
#include "quill_line.h"

Tokenizer tokenizer_init(Line *line) {
  Tokenizer tokenizer;
  memset(&tokenizer, 0, sizeof(Tokenizer));
  tokenizer.current = 0;
  tokenizer.size = line_size(line);
  return tokenizer;
}

bool tokenizer_next_token(Tokenizer *tokenizer, Token *token) {

  u8 codepoit = line_get_codepoint_at(tokenizer->line, tokenizer->current);
  u8 next_codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current + 1);

  while(tokenizer->current < tokenizer->size) {
    switch(codepoit) {

    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': {
      return tokenizer_parse_number(tokenizer, token);
    } break;

    /* TODO: Use a map to parse utf8 codepoints */
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': {
      return tokenizer_parse_word(tokenizer, token);
    } break;

    case '"': {
      return tokenizer_parse_string(tokenizer, token);
    } break;

    case '/': {
      if((next_codepoint == '/') || (next_codepoint == '*')) {
        return tokenizer_parse_comment(tokenizer, token);
      }
    } break;

    case '#': {
      return tokenizer_parse_preprocessor(tokenizer, token);
    } break;

    }

    codepoit = line_get_codepoint_at(tokenizer->line, tokenizer->current);
    next_codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current + 1);
  }

  return false;
}

bool tokenizer_parse_number(Tokenizer *tokenizer, Token *token) {
  (void)tokenizer; (void)token;
  return false;
}

bool tokenizer_parse_word(Tokenizer *tokenizer, Token *token) {
  (void)tokenizer; (void)token;
  return false;
}

bool tokenizer_parse_string(Tokenizer *tokenizer, Token *token) {
  (void)tokenizer; (void)token;
  return false;
}

bool tokenizer_parse_comment(Tokenizer *tokenizer, Token *token) {
  (void)tokenizer; (void)token;
  return false;
}

bool tokenizer_parse_preprocessor(Tokenizer *tokenizer, Token *token) {
  (void)tokenizer; (void)token;
  return false;
}

