#define _POSIX_C_SOURCE 200809L
#include "lexer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    char *buf;     
    size_t len;
    size_t pos;
    int interactive;
    lex_more_fn more;
    void *ctx;
    const char *err;
} lexer;

typedef struct {
    char *data;
    size_t len, cap;
} strbuf;


static int is_meta(int c)
{
    return c != 0 && strchr(" \t\n|&;()<>", c) != NULL;
}

static int sb_push(strbuf *sb, char c)
{
    if (sb->len + 1 >= sb->cap) {
        size_t ncap = sb->cap ? sb->cap * 2 : 32;
        char *p = realloc(sb->data, ncap);
        if (!p)
            return -1;
        sb->data = p;
        sb->cap = ncap;
    }
    sb->data[sb->len++] = c;
    return 0;
}

static int add_token(token_list *tl, token_type type, char *text)
{
    if (tl->len == tl->cap) {
        size_t ncap = tl->cap ? tl->cap * 2 : 16;
        token *p = realloc(tl->items, ncap * sizeof *p);
        if (!p)
            return -1;
        tl->items = p;
        tl->cap = ncap;
    }
    tl->items[tl->len].type = type;
    tl->items[tl->len].text = text;
    tl->len++;
    return 0;
}

static int fill(lexer *L)
{
    if (!L->more)
        return 0;
    char *line = L->more(L->ctx);
    if (!line)
        return 0;

    size_t n = strlen(line);
    int need_nl = (n == 0 || line[n - 1] != '\n');
    char *p = realloc(L->buf, L->len + n + need_nl + 1);
    if (!p) {
        free(line);
        L->err = "недостаточно памяти";
        return -1;
    }
    L->buf = p;
    memcpy(L->buf + L->len, line, n);
    L->len += n;
    if (need_nl)
        L->buf[L->len++] = '\n';
    L->buf[L->len] = '\0';
    free(line);
    return 1;
}

static int more_in_quote(lexer *L, char q)
{
    if (L->interactive) {
        int r = fill(L);
        if (r > 0)
            return 0;
        if (r < 0)
            return -1;
    }
    L->err = (q == '\'') ? "незакрытая одинарная кавычка"
                         : "незакрытая двойная кавычка";
    return -1;
}


static int lex_operator(lexer *L, token_list *out)
{
    char c = L->buf[L->pos];
    char n = L->buf[L->pos + 1];
    token_type t;
    size_t w = 1;

    switch (c) {
    case '|': if (n == '|') { t = TOK_OR;     w = 2; } else t = TOK_PIPE; break;
    case '&': if (n == '&') { t = TOK_AND;    w = 2; } else t = TOK_AMP;  break;
    case '>': if (n == '>') { t = TOK_APPEND; w = 2; } else t = TOK_GREAT; break;
    case '<': t = TOK_LESS;   break;
    case ';': t = TOK_SEMI;   break;
    case '(': t = TOK_LPAREN; break;
    default:  t = TOK_RPAREN; break;   /* ')' */
    }
    L->pos += w;
    if (add_token(out, t, NULL) < 0) {
        L->err = "недостаточно памяти";
        return -1;
    }
    return 0;
}


static int lex_word(lexer *L, token_list *out)
{
    strbuf sb = {0};
    int started = 0;   /

    for (;;) {
        int c = (unsigned char)L->buf[L->pos];
        if (c == 0 || is_meta(c))
            break;

        if (c == '\\') {
            char n = L->buf[L->pos + 1];
            if (n == '\n') {        
                L->pos += 2;
                if (L->buf[L->pos] == 0 && fill(L) < 0)
                    goto fail;
                continue;
            }
            started = 1;
            if (n == 0) {
                if (sb_push(&sb, '\\') < 0) goto oom;
                L->pos++;
            } else {
                if (sb_push(&sb, n) < 0) goto oom;
                L->pos += 2;
            }
            continue;
        }

        if (c == '\'') {
            started = 1;
            L->pos++;
            for (;;) {
                c = (unsigned char)L->buf[L->pos];
                if (c == 0) {
                    if (more_in_quote(L, '\'') < 0)
                        goto fail;
                    continue;
                }
                L->pos++;
                if (c == '\'')
                    break;
                if (sb_push(&sb, (char)c) < 0) goto oom;
            }
            continue;
        }

        if (c == '"') {
            started = 1;
            L->pos++;
            for (;;) {
                c = (unsigned char)L->buf[L->pos];
                if (c == 0) {
                    if (more_in_quote(L, '"') < 0)
                        goto fail;
                    continue;
                }
                if (c == '"') {
                    L->pos++;
                    break;
                }
                if (c == '\\') {
                    char n = L->buf[L->pos + 1];
                    if (n == '"' || n == '\\') {
                        if (sb_push(&sb, n) < 0) goto oom;
                        L->pos += 2;
                    } else if (n == '\n') {
                        L->pos += 2;
                    } else {        
                        if (sb_push(&sb, '\\') < 0) goto oom;
                        L->pos++;
                    }
                    continue;
                }
                if (sb_push(&sb, (char)c) < 0) goto oom;
                L->pos++;
            }
            continue;
        }

     
        started = 1;
        if (sb_push(&sb, (char)c) < 0) goto oom;
        L->pos++;
    }

    if (!started) {         
        free(sb.data);
        return 0;
    }
    if (sb_push(&sb, '\0') < 0) goto oom;
    if (add_token(out, TOK_WORD, sb.data) < 0) {
        free(sb.data);
        L->err = "недостаточно памяти";
        return -1;
    }
    return 0;

oom:
    L->err = "недостаточно памяти";
fail:
    free(sb.data);
    return -1;
}



int lex_line(const char *line, int interactive, lex_more_fn more, void *ctx,
             token_list *out, const char **err)
{
    lexer L = {0};
    size_t n = strlen(line);
    int need_nl = (n == 0 || line[n - 1] != '\n');

    out->items = NULL;
    out->len = out->cap = 0;

    L.buf = malloc(n + need_nl + 1);
    if (!L.buf) {
        if (err) *err = "недостаточно памяти";
        return -1;
    }
    memcpy(L.buf, line, n);
    L.len = n;
    if (need_nl)
        L.buf[L.len++] = '\n';
    L.buf[L.len] = '\0';
    L.interactive = interactive;
    L.more = more;
    L.ctx = ctx;

    for (;;) {
        int c = (unsigned char)L.buf[L.pos];
        if (c == 0)
            break;
        if (c == ' ' || c == '\t' || c == '\n') {
            L.pos++;
            continue;
        }
        if (c == '#') {
            while (L.buf[L.pos] && L.buf[L.pos] != '\n')
                L.pos++;
            continue;
        }
        if (is_meta(c)) {
            if (lex_operator(&L, out) < 0)
                goto fail;
        } else {
            if (lex_word(&L, out) < 0)
                goto fail;
        }
    }

    free(L.buf);
    return 0;

fail:
    if (err) *err = L.err;
    free(L.buf);
    token_list_free(out);
    return -1;
}

void token_list_free(token_list *tl)
{
    for (size_t i = 0; i < tl->len; i++)
        free(tl->items[i].text);
    free(tl->items);
    tl->items = NULL;
    tl->len = tl->cap = 0;
}

#ifdef LEXER_DEMO
#include <stdio.h>
#include <unistd.h>

static const char *names[] = {
    "WORD", "PIPE", "OR", "AMP", "AND", "SEMI",
    "LPAREN", "RPAREN", "LESS", "GREAT", "APPEND"
};

static char *read_cont(void *ctx)
{
    int interactive = *(int *)ctx;
    char *line = NULL;
    size_t cap = 0;
    if (interactive) {
        fputs("> ", stdout);
        fflush(stdout);
    }
    if (getline(&line, &cap, stdin) < 0) {
        free(line);
        return NULL;
    }
    return line;
}

int main(void){
    int interactive = isatty(STDIN_FILENO);
    char *line = NULL;
    size_t cap = 0;

    for (;;) {
        if (interactive) {
            fputs("$ ", stdout);
            fflush(stdout);
        }
        if (getline(&line, &cap, stdin) < 0)
            break;

        token_list tl;
        const char *err = NULL;
        if (lex_line(line, interactive, read_cont, &interactive, &tl, &err) < 0) {
            fprintf(stderr, "lexer: %s\n", err);
            continue;
        }
        for (size_t i = 0; i < tl.len; i++) {
            if (tl.items[i].type == TOK_WORD)
                printf("%s[%s]\n", names[TOK_WORD], tl.items[i].text);
            else
                printf("%s\n", names[tl.items[i].type]);
        }
        token_list_free(&tl);
    }
    free(line);
    return 0;
}
#endif
