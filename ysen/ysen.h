#ifndef YSEN_H
#define YSEN_H
#include <stddef.h>

// Lex options
#define YSEN_LEX_COMMENTS 1
#define YSEN_LEX_WHITESPACE 2

// Error codes
#define YSEN_ERROR_OK 0
#define YSEN_ERROR_INVALID_FLOAT 1

// Ysen function
#define YSENFUN

// Flags
#define YSEN_SET(v, flag) v |= flag
#define YSEN_UNSET(v, flag) v &= ~(flag)
#define YSEN_ISSET(v, flag) ((v & (flag)) == (flag))

#define YSEN_FLAG_REFSTR 		1 << 1
#define YSEN_FLAG_NEGNUM 		1 << 2
#define YSEN_FLAG_FLOATNUM 		1 << 3

typedef enum ysen_token_type ysen_token_type;
enum ysen_token_type {
	ysen_token_none,
	ysen_token_comment,
	ysen_token_whitespace,
	ysen_token_identifier,
	ysen_token_keyword,
	ysen_token_string,
	ysen_token_number,
	ysen_token_operator,
	ysen_token_dot,
	ysen_token_comma,
	ysen_token_colon,
	ysen_token_semicolon,
	ysen_token_lbracket,
	ysen_token_rbracket,
	ysen_token_lsquiggly,
	ysen_token_rsquiggly,
	ysen_token_lparen,
	ysen_token_rparen,
};

typedef struct ysen_string_t ysen_string_t;
struct ysen_string_t {
	char* buf;
	size_t len;
	int flags; // REFSTR = no free() 
};

typedef struct ysen_pos_t ysen_pos_t;
struct ysen_pos_t {
	size_t row;
	size_t col;
	size_t ofs;
};

typedef struct ysen_range_t ysen_range_t;
struct ysen_range_t {
	ysen_pos_t* start;
	ysen_pos_t* end;
};

typedef struct ysen_token_t ysen_token_t;
struct ysen_token_t {
	struct ysen_token_t* next;
	ysen_token_type type;
	ysen_string_t* content;	
	ysen_range_t* source_range;
	int flags;
};

/* YSEN string functions */
YSENFUN ysen_string_t* 		ysen_string_create(void);
YSENFUN ysen_string_t* 		ysen_string_clone(const char*);
YSENFUN ysen_string_t* 		ysen_string_refstr(const char*);
YSENFUN ysen_string_t* 		ysen_string_adopt(char*);
YSENFUN ysen_string_t* 		ysen_string_substr(const char*, size_t ofs, size_t len);
YSENFUN void 				ysen_string_assign(ysen_string_t*, const char*); // clones str
YSENFUN void				ysen_string_assign_refstr(ysen_string_t*, const char*); // stores passed string as ref
YSENFUN void 				ysen_string_free(ysen_string_t*);

/* Pos & range functions */
YSENFUN ysen_pos_t* 		ysen_pos_create(void);
YSENFUN ysen_pos_t* 		ysen_pos_clone(ysen_pos_t*);
YSENFUN void				ysen_pos_free(ysen_pos_t*);
YSENFUN ysen_range_t* 		ysen_range_create(void);
YSENFUN ysen_range_t* 		ysen_range_with(ysen_pos_t* start, ysen_pos_t* end); // clones start & end.
YSENFUN ysen_range_t*		ysen_range_adopt(ysen_pos_t* start, ysen_pos_t* end); // adopts start & end.
YSENFUN int 				ysen_range_contains(ysen_range_t*, ysen_pos_t*);
YSENFUN void				ysen_range_free(ysen_range_t*);

/* YSEN lexer */
YSENFUN ysen_token_t*		ysen_lex(const char* content);
YSENFUN const char*			ysen_token_type_string(ysen_token_t*); // Get a friendly string type.
YSENFUN void				ysen_token_free(ysen_token_t*);
YSENFUN int					ysen_lex_error_code(void);
YSENFUN const char*			ysen_lex_error_string(void);
YSENFUN void				ysen_lex_setopt(int opt, int v);
YSENFUN int					ysen_lex_getopt(int opt);

/* YSEN parser */
//YSENFUN ysen_ast_t*			ysen_parse(ysen_token_t* tokens);

#endif
