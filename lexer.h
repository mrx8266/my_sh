#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    TOK_WORD,    /* слово (text != NULL, кавычки уже сняты) */
    TOK_PIPE,    /* |  */
    TOK_OR,      /* || */
    TOK_AMP,     /* &  */
    TOK_AND,     /* && */
    TOK_SEMI,    /* ;  */
    TOK_LPAREN,  /* (  */
    TOK_RPAREN,  /* )  */
    TOK_LESS,    /* <  */
    TOK_GREAT,   /* >  */
    TOK_APPEND   /* >> */
} token_type;

typedef struct {
    token_type type;
    char *text;          /* только для TOK_WORD, иначе NULL */
} token;

typedef struct {
    token *items;
    size_t len;
    size_t cap;
} token_list;

typedef char *(*lex_more_fn)(void *ctx);

int lex_line(const char *line, int interactive, lex_more_fn more, void *ctx,
             token_list *out, const char **err);

void token_list_free(token_list *tl);

#endif

