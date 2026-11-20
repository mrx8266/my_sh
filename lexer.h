#ifndef
#define HAS_SINGLE 1
#define HAS_DOUBLE 2
#define HAS_ESCAPE 4
#endif

typedef struct {
    TokType type;
    union {
        struct {
            char *text;  
            int   flags;
        } word;
        OpKind op;
    };
    int line;          
} Token;

typedef enum {
    TOK_WORD,
    TOK_OP,
    TOK_NEWLINE,
    TOK_EOF,
} TokType;

typedef enum {
    OP_PIPE,        // |
    OP_OR,          // ||
    OP_AND,         // &&
    OP_AMP,         // &
    OP_SEMI,        // ;
    OP_LPAREN, OP_RPAREN,
    OP_LBRACE, OP_RBRACE,
    OP_REDIR_IN,    // <
    OP_REDIR_OUT,   // >
    OP_REDIR_APP,   // >>
    OP_HEREDOC,     // <<
    OP_REDIR_ERR,   // 2>
    OP_REDIR_ERR_APP, // 2>>
    OP_DUP_OUT,     // >&
    OP_DUP_ERR,     // 2>&1 (упрощённо)
} OpKind;

typedef enum {
    Q_NONE,     // без кавычек
    Q_SINGLE,   // '...'
    Q_DOiUBLE,   // "..."
    Q_MIXED,    // часть в кавычках, часть нет — храним флаги
} Quoting;

typedef struct {
    Token *items;
    size_t len, cap;
} TokenVec;

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
    int line;
    TokenVec out;
    int error;
    char errmsg[256];
} Lexer;
