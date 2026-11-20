struct Token;
enum Token_type;


typedef struct Token{
	char* mess;
	Token_type type;
}Token;

typedef enum Token_type{
	WORD,
	OPERATOR,
	REDIR,
	NEWLINE,
	ASG_WORD,
	RSRV_WORD,
	EOF,
}Token_type;


