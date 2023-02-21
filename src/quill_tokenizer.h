#ifndef _QUILL_TOKENIZER_H_
#define _QUILL_TOKENIZER_H_

#include "quill.h"

struct Line;

typedef enum TokenType {
  TOKEN_TYPE_UNKNOWN,
  TOKEN_TYPE_WORD,
  TOKEN_TYPE_KEYWORD,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_NUBER,
  TOKEN_TYPE_COMMENT,

  TOKEN_TYPE_COUNT,
} TokenType;

typedef struct Token {
  TokenType type;
  u32 start;
  u32 end;
  struct Line *line;
} Token;

void token_print(Token token);

typedef struct Tokenizer {

  struct Line *line;
  u32 current;
  u32 size;

  bool on_comment;
  bool on_preprocessor;

} Tokenizer;

Tokenizer tokenizer_init(struct Line *line);
void tokenizer_set_line(struct Line *line);
bool tokenizer_next_token(Tokenizer *tokenizer, Token *token);

bool tokenizer_parse_number(Tokenizer *tokenizer, Token *token);
bool tokenizer_parse_word(Tokenizer *tokenizer, Token *token);
bool tokenizer_parse_string(Tokenizer *tokenizer, Token *token);
bool tokenizer_parse_multiline_comment(Tokenizer *tokenizer, Token *token);
bool tokenizer_parse_comment(Tokenizer *tokenizer, Token *token);
bool tokenizer_parse_unknown(Tokenizer *tokenizer, Token *token);


#endif /* _QUILL_TOKENIZER_H_ */
