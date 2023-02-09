#include "quill_tokenizer.h"
#include "quill_line.h"

static char *token_type_to_string[TOKEN_TYPE_COUNT] = {
  "TOKEN_TYPE_UNKNOWN",
  "TOKEN_TYPE_WORD",
  "TOKEN_TYPE_KEYWORD",
  "TOKEN_TYPE_STRING",
  "TOKEN_TYPE_NUBER",
  "TOKEN_TYPE_COMMENT"
};

void token_print(Token token) {
  printf("line size:%d, token.end:%d\n", line_size(token.line), token.end);


  printf("Token Type: %s\n", token_type_to_string[token.type]);
  printf("Token Content: ");
  for(u32 i = token.start; i < token.end; ++i) {
    u8 codepoint = line_get_codepoint_at(token.line, i);
    printf("%c", codepoint);
  }
  printf("\n");
}


Tokenizer tokenizer_init(Line *line) {
  Tokenizer tokenizer;
  memset(&tokenizer, 0, sizeof(Tokenizer));
  tokenizer.line = line;
  tokenizer.current = 0;
  tokenizer.size = line_size(line);
  return tokenizer;
}

void tokenier_set_line(Tokenizer *tokenizer, Line *line) {
  tokenizer->line = line;
  tokenizer->current = 0;
  tokenizer->size = line_size(line);
}

static inline bool is_digit(u8 codepoint) {
  return (codepoint >= '0') && (codepoint <= '9');
}

static inline bool is_alpha(u8 codepoint) {
/* TODO: Need to take in consideration UTF8 codepoints */
  return ((codepoint >= 'A') && (codepoint <= 'Z')) ||
      ((codepoint >= 'a') && (codepoint <= 'z'));
}

bool tokenizer_next_token(Tokenizer *tokenizer, Token *token) {
  if(tokenizer->current == tokenizer->size) {
    return false;
  }

  token->line = tokenizer->line;
  u8 codepoit = line_get_codepoint_at(tokenizer->line, tokenizer->current);

  while(tokenizer->current < tokenizer->size) {
    u8 next_codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current + 1);

    if(tokenizer->on_comment) {
      return tokenizer_parse_comment(tokenizer, token);
    }

    switch(codepoit) {

    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': {
      return tokenizer_parse_number(tokenizer, token);
    } break;

    case '.': {
      if(is_digit(next_codepoint)) {
        tokenizer_parse_number(tokenizer, token);
      }
    } break;

    /* TODO: Use a map to parse utf8 codepoints */
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
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
    default: {
      return tokenizer_parse_unknown(tokenizer, token);
    } break;
    }

    if(tokenizer->current == (tokenizer->size - 1)) {
      break;
    }

    codepoit = line_get_codepoint_at(tokenizer->line, tokenizer->current);
  }

  return false;
}

bool tokenizer_parse_number(Tokenizer *tokenizer, Token *token) {
  u32 start = tokenizer->current;
  u8 codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current++);

  while((is_digit(codepoint)) || (codepoint == '.')) {
    codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current);
    ++tokenizer->current;
  }

  if(codepoint == 'f') {
    ++tokenizer->current;
  }

  token->start = start;
  token->end = tokenizer->current;
  token->type = TOKEN_TYPE_NUBER;

  return true;
}

bool tokenizer_parse_word(Tokenizer *tokenizer, Token *token) {
  u32 start = tokenizer->current;
  u8 codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current++);

  while(is_alpha(codepoint) || (codepoint == '_') || is_digit(codepoint)) {
    codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current);
    ++tokenizer->current;
  }

  token->start = start;
  token->end = tokenizer->current;
  token->type = TOKEN_TYPE_WORD;

  return true;
}

bool tokenizer_parse_string(Tokenizer *tokenizer, Token *token) {
  u32 start = tokenizer->current;
  u8 codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current++);

  assert(codepoint == '"');
  codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current++);

  while(codepoint != '"') {
    codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current);
    ++tokenizer->current;
  }

  token->start = start;
  token->end = tokenizer->current;
  token->type = TOKEN_TYPE_STRING;
  return true;
}

static bool tokenizer_parse_multiline_comment(Tokenizer *tokenizer, Token *token) {
  u32 start = tokenizer->current;
  while(tokenizer->current < tokenizer->size) {
    u8 codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current++);
    u8 next_codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current);

    if((codepoint == '*') && (next_codepoint == '/')) {
      tokenizer->on_comment = false;
      ++tokenizer->current;
      break;
    }

    ++tokenizer->current;
  }

  token->start = start;
  token->end = tokenizer->current;
  token->type = TOKEN_TYPE_COMMENT;
  return true;
}

bool tokenizer_parse_comment(Tokenizer *tokenizer, Token *token) {
  u32 start = tokenizer->current;

  if(tokenizer->on_comment) {
    return tokenizer_parse_multiline_comment(tokenizer, token);
  }

  u8 codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current++);
  assert(codepoint == '/');
  codepoint = line_get_codepoint_at(tokenizer->line, tokenizer->current);

  if(codepoint == '*') {
    tokenizer->on_comment = true;
    return tokenizer_parse_multiline_comment(tokenizer, token);
  } else if(codepoint == '/') {
    tokenizer->current = tokenizer->size;
  }

  token->start = start;
  token->end = tokenizer->current;
  token->type = TOKEN_TYPE_COMMENT;
  return true;
}

bool tokenizer_parse_unknown(Tokenizer *tokenizer, Token *token) {
  token->start = tokenizer->current++;
  token->end = tokenizer->current;
  token->type = TOKEN_TYPE_UNKNOWN;
  return true;
}

