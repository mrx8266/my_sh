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

/*
 * Функция дочитывания: возвращает следующую строку (malloc, лексер сам
 * вызовет free) или NULL при конце ввода. В интерактивном режиме именно
 * здесь печатается вторичное приглашение (PS2).
 */
typedef char *(*lex_more_fn)(void *ctx);

/*
 * Разбирает строку line на токены.
 *   interactive — если 1, незакрытая кавычка не ошибка: лексер просит
 *                 следующую строку через more и продолжает.
 *   more        — может быть NULL (тогда продолжений строки нет).
 *                 Продолжение "\<перевод строки>" использует more
 *                 в обоих режимах.
 * Возврат: 0 — успех, -1 — ошибка (*err указывает на статическое сообщение).
 * При успехе список нужно освободить token_list_free().
 */
int lex_line(const char *line, int interactive, lex_more_fn more, void *ctx,
             token_list *out, const char **err);

void token_list_free(token_list *tl);

#endif

