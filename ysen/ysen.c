#include "ysen.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define YSEN_MALLOC(type) malloc(type)
#define YSEN_RSUBSTR(buf, range) ysen_string_substr(buf, range->start->ofs, range->end->ofs - range->start->ofs)
int ysen_options[24];

int isop(char c)
{
	return c == '=' || c == '+' || c == '-' ||
		c == '*' || c == '/' || c == '%' || 
		c == '&' || c == '|' || c == '^' ||
		c == '>' || c == '<' || c == '!' ||
		c == '~' || c == '?';
}

/* YSEN string functions */
void ysen_string_free_buf(ysen_string_t* s) {
	if (s->buf && !YSEN_ISSET(s->flags, YSEN_FLAG_REFSTR)) {
		free(s->buf); // Only free buf if it is not a refstr.
		s->buf = NULL;
		s->len = 0;
	}
}

ysen_string_t* ysen_string_create(void) {
	ysen_string_t* s = YSEN_MALLOC(sizeof(ysen_string_t));
	if (!s) return NULL;
	s->buf = NULL;
	s->len = 0;
	s->flags = 0;
	return s;	
}

ysen_string_t* ysen_string_clone(const char* str) {
	ysen_string_t* s = ysen_string_create();
	if (!s) return NULL;
	s->len = strlen(str);
	s->buf = YSEN_MALLOC(s->len + 1);
	if (!s->buf) {
		ysen_string_free(s);
		return NULL;
	}
	s->buf[s->len] = 0;
	memcpy(s->buf, str, s->len);
	return s;
}

ysen_string_t* ysen_string_refstr(const char* str) {
	ysen_string_t* s = ysen_string_create();
	if (!s) return NULL;
	s->len = strlen(str);
	s->buf = (char*)str; // This looks evil, but we are setting the flag REFSTR meaning we won't allow any type of modification or freeing of the buffer.
	YSEN_SET(s->flags, YSEN_FLAG_REFSTR);
	return s;
}

ysen_string_t* ysen_string_adopt(char* str) {
	ysen_string_t* s = ysen_string_create();
	if (!s) return NULL;
	s->len = strlen(str);
	s->buf = str;
	return s;

}

ysen_string_t* ysen_string_substr(const char* str, size_t ofs, size_t len) {
	ysen_string_t* s = ysen_string_create();
	if (!s) return NULL;
	s->len = len;
	s->buf = YSEN_MALLOC(s->len + 1);
	if (!s->buf) {
		ysen_string_free(s);
		return NULL;
	}
	s->buf[s->len] = 0;
	memcpy(s->buf, str + ofs, len);
	return s;

}

void ysen_string_assign(ysen_string_t* s, const char* buf)
{
	ysen_string_free_buf(s);

	s->len = strlen(buf);
	s->buf = YSEN_MALLOC(s->len + 1);
	if (!s->buf) return; // we failed
	s->buf[s->len] = 0;
	memcpy(s->buf, buf, s->len);
}

void ysen_string_assign_refstr(ysen_string_t* s, const char* buf)
{
	ysen_string_free_buf(s);

	s->len = strlen(buf);
	s->buf = (char*)buf;
	YSEN_SET(s->flags, YSEN_FLAG_REFSTR);
}

void ysen_string_free(ysen_string_t* s) {
	if (!s) return;

	ysen_string_free_buf(s); // free buf if needed.
	free(s);
}

/* YSEN pos & rane functions */
ysen_pos_t* ysen_pos_create(void)
{
	ysen_pos_t* pos = YSEN_MALLOC(sizeof(ysen_pos_t));
	if (!pos) return NULL;
	pos->row = 0;
	pos->col = 0;
	pos->ofs = 0;
	return pos;
}

ysen_pos_t* ysen_pos_clone(ysen_pos_t* p)
{
	ysen_pos_t* pos = ysen_pos_create();
	if (!pos) return NULL;
	pos->row = p->row;
	pos->col = p->col;
	pos->ofs = p->ofs;
	return pos;
}

void ysen_pos_free(ysen_pos_t* p)
{
	if (p) free(p);
}

ysen_range_t* ysen_range_create()
{
	ysen_range_t* range = YSEN_MALLOC(sizeof(ysen_range_t));
	if (!range) return NULL;
	range->start = NULL;
	range->end = NULL;
	return range;
}

ysen_range_t* ysen_range_with(ysen_pos_t* start, ysen_pos_t* end)
{
	ysen_range_t* range = YSEN_MALLOC(sizeof(ysen_range_t));
	if (!range) return NULL;
	range->start = ysen_pos_clone(start);
	range->end = ysen_pos_clone(end);
	return range;
}

ysen_range_t* ysen_range_adopt(ysen_pos_t* start, ysen_pos_t* end)
{
	ysen_range_t* range = YSEN_MALLOC(sizeof(ysen_range_t));
	if (!range) return NULL;
	range->start = start;
	range->end = end;
	return range;
}

int ysen_range_contains(ysen_range_t* range, ysen_pos_t* position)
{
	return (position->row > range->start->row || (position->row == range->start->row && position->col >= range->start->col)) &&
		(position->row < range->start->row || (position->row == range->end->row && position->col <= range->end->col));
}

void ysen_range_free(ysen_range_t* range)
{
	if (!range) return;
	if (range->start) ysen_pos_free(range->start);
	if (range->end) ysen_pos_free(range->end);

	free(range);
}

/* YSEN lexer */
typedef struct ysen_lexer_t ysen_lexer_t;
struct ysen_lexer_t {
	const char* buf;
	size_t len;
	ysen_pos_t* pos;
	ysen_token_t* root_token;
	ysen_token_t* tail_token;	
};

char ysen_peek(ysen_lexer_t* l, int o)
{
	if (l->pos->ofs + o < l->len) return l->buf[l->pos->ofs + o];
	return 0;
}

int ysen_eof(ysen_lexer_t* l, int o)
{
	return l->pos->ofs + o >= l->len;
}

char ysen_inc(ysen_lexer_t* l)
{
	if (ysen_eof(l, 0)) return 0;

	char c = ysen_peek(l, 0);
	++l->pos->col;
	++l->pos->ofs;
	if (c == '\n') {
		++l->pos->row;
		l->pos->col = 0;
	}

	return ysen_peek(l, 0);
}

void ysen_lex_append(ysen_lexer_t* l, ysen_token_t* tok)
{
	if (l->root_token == NULL) {
		l->root_token = tok;
		l->tail_token = tok;
	}
	else {
		l->tail_token->next = tok;
		l->tail_token = tok;
	}
}

ysen_token_t* ysen_lex_impl(ysen_lexer_t*);

void ysen_token_free(ysen_token_t* t)
{
	if (!t) return;

	ysen_token_t* n = t->next;
	while (n != NULL) {
		ysen_token_t* tmp = n->next;
		n->next = NULL; // when we free n it should not free its siblings.
		ysen_token_free(n);
		n = tmp; // next
	}

	if (t->content) ysen_string_free(t->content);
	if (t->source_range) ysen_range_free(t->source_range);
	free(t);
}

int ysen_lex_error_code()
{
	return ysen_options[0];
}

const char* ysen_lex_error_string()
{
	return "ok";
}

void ysen_lex_setopt(int opt, int v)
{
	ysen_options[opt] = v;
}

int ysen_lex_getopt(int opt)
{
	return ysen_options[opt];
}

const char* ysen_token_type_string(ysen_token_t* tok)
{
	switch (tok->type) {
		case ysen_token_whitespace: return "space";
		case ysen_token_comment: return "comment";
		case ysen_token_identifier: return "identifier";
		case ysen_token_keyword: return "keyword";
		case ysen_token_string: return "string";		 
		case ysen_token_number: return "number";		 
		case ysen_token_operator: return "operator";		 
		case ysen_token_dot: return "dot";		 
		case ysen_token_comma: return "comma";		 
		case ysen_token_colon: return "colon";		 
		case ysen_token_semicolon: return "semicolon";		 
		case ysen_token_lbracket: return "lbracket";		 
		case ysen_token_rbracket: return "rbracket";		 
		case ysen_token_lsquiggly: return "lsquiggly";		 
		case ysen_token_rsquiggly: return "rsquiggly";		 
		case ysen_token_lparen: return "lparen";		 
		case ysen_token_rparen: return "rparen";		 
		default: break;
	}

	return "none";
}

ysen_token_t* ysen_lex(const char* content)
{
	if (!content) return NULL;
	ysen_lexer_t lexer;
	lexer.buf = content;
	lexer.len = strlen(content);
	lexer.pos = ysen_pos_create();
	lexer.root_token = NULL;
	lexer.tail_token = NULL;
	ysen_token_t* tok = ysen_lex_impl(&lexer);
	ysen_pos_free(lexer.pos);
	return tok;
}

ysen_token_t* ysen_token_create()
{
	ysen_token_t* tok = YSEN_MALLOC(sizeof(ysen_token_t));
	if (!tok) return NULL;
	tok->next = NULL;
	tok->type = ysen_token_none;
	tok->content = NULL;
	tok->source_range = NULL;
	tok->flags = 0;
	return tok;
}

ysen_string_t* ysen_lexer_consume_str(ysen_lexer_t* l, ysen_range_t** out_range)
{
	*out_range = ysen_range_create();
	ysen_range_t* r = *out_range;
	r->start = ysen_pos_clone(l->pos);

	// Inc '
	ysen_inc(l);
	char c = ysen_peek(l, 0);
	do {
		if (c == '\\') {
			// escape seq
			ysen_inc(l);
		}
		ysen_inc(l);	
	} while (!ysen_eof(l, 0) && (c = ysen_peek(l, 0)) != '\''); 
	
	if (ysen_eof(l, 0)) return NULL;
	ysen_inc(l); // Inc '
	r->end = ysen_pos_clone(l->pos);

	return ysen_string_substr(l->buf, r->start->ofs + 1, (r->end->ofs - 1) - (r->start->ofs + 1));
}

ysen_string_t* ysen_lexer_consume_digits(ysen_lexer_t* l, ysen_range_t** out_range, int* out_flags)
{
	*out_range = ysen_range_create();
	ysen_range_t* r = *out_range;
	r->start = ysen_pos_clone(l->pos);
	
	char c = ysen_peek(l, 0);
	
	if (c == '-') {
		YSEN_SET(*out_flags, YSEN_FLAG_NEGNUM);
	}

	do {
		if (c == '.') {
			if (YSEN_ISSET(*out_flags, YSEN_FLAG_FLOATNUM)) {
				printf("%d %d\n", *out_flags, YSEN_FLAG_FLOATNUM);
				fprintf(stderr, "floatnum was already set\n");
				return NULL; // TODO error code
			}

			YSEN_SET(*out_flags, YSEN_FLAG_FLOATNUM);
		}
		c = ysen_inc(l);
	} while(!ysen_eof(l, 0) && (isdigit(c) || c == '.'));

	r->end = ysen_pos_clone(l->pos);
	return ysen_string_substr(l->buf, r->start->ofs, r->end->ofs - r->start->ofs);
}

ysen_string_t* ysen_lexer_consume_ident(ysen_lexer_t* l, ysen_range_t** out_range)
{
	*out_range = ysen_range_create();
	ysen_range_t* r = *out_range;
	r->start = ysen_pos_clone(l->pos);

	char c = ysen_peek(l, 0);
	do {	
		c = ysen_inc(l);
	} while(!ysen_eof(l, 0) && (isalnum(c) || c == '_'));

	r->end = ysen_pos_clone(l->pos);

	return ysen_string_substr(l->buf, r->start->ofs, r->end->ofs - r->start->ofs);
}

ysen_string_t* ysen_lexer_consume_op(ysen_lexer_t* l, ysen_range_t** out_range)
{
	#define MAKE_ONEC(c1)									\
		if (c == c1) {										\
			char* buf = malloc(2);							\
			buf[0] = c;										\
			buf[1] = 0;										\
			r->end = ysen_pos_clone(r->start);				\
			return ysen_string_adopt(buf);					\
		}

	#define MAKE_TWOC(c1, c2)								\
		if (c == c1 && cn == c2) {							\
			char* buf = malloc(3);							\
		   	buf[0] = c1;									\
			buf[1] = c2;									\
			buf[2] = 0;										\
			r->end = ysen_pos_clone(l->pos);				\
			ysen_inc(l);									\
			return ysen_string_adopt(buf);					\
		}

	*out_range = ysen_range_create();
	ysen_range_t* r = *out_range;
	r->start = ysen_pos_clone(l->pos);

	char c = ysen_peek(l, 0); // curr char
	char cn = ysen_inc(l); // next char
	
	MAKE_TWOC('+', '+')
	MAKE_TWOC('-', '-')
	MAKE_TWOC('=', '=')
	MAKE_TWOC('>', '=')
	MAKE_TWOC('<', '=')
	MAKE_TWOC('&', '&')
	MAKE_TWOC('|', '|')
	MAKE_TWOC('|', '=')
	MAKE_TWOC('&', '=')
	MAKE_TWOC('^', '=')
	MAKE_TWOC('~', '=')
	MAKE_TWOC('>', '>')
	MAKE_TWOC('<', '<')
	MAKE_TWOC('-', '>')
	MAKE_TWOC('+', '=')
	MAKE_TWOC('-', '=')
	MAKE_TWOC('*', '=')
	MAKE_TWOC('/', '=')
	MAKE_TWOC('%', '=')
	MAKE_TWOC('!', '=')
	MAKE_ONEC('+')
	MAKE_ONEC('-')
	MAKE_ONEC('*')
	MAKE_ONEC('/')
	MAKE_ONEC('%')
	MAKE_ONEC('&')
	MAKE_ONEC('|')
	MAKE_ONEC('^')
	MAKE_ONEC('~')
	MAKE_ONEC('?')
	MAKE_ONEC('>')
	MAKE_ONEC('<')
	MAKE_ONEC('!') 
	MAKE_ONEC('=')

	return NULL;
}

void ysen_consume_ws(ysen_lexer_t* l)
{
	char c = ysen_peek(l, 0);
	if (!isspace(c)) return;

	ysen_pos_t* start = NULL;
	if (ysen_lex_getopt(YSEN_LEX_WHITESPACE)) {
		start = ysen_pos_clone(l->pos);
	}

	do {
		c = ysen_inc(l);
	} while (!ysen_eof(l, 0) && isspace(c));

	if (start) {
		// Create the token
		ysen_token_t* tok = ysen_token_create();
		tok->type = ysen_token_whitespace;
		tok->source_range = ysen_range_adopt(start, ysen_pos_clone(l->pos));
		tok->content = YSEN_RSUBSTR(l->buf, tok->source_range);
	   	ysen_lex_append(l, tok);
	}
}

void ysen_consume_comments(ysen_lexer_t* l)
{
	int iter = 0;
	while (!ysen_eof(l, 0)) {
		ysen_consume_ws(l);
		char c = ysen_peek(l, 0);
		char cn = ysen_peek(l, 1);
		int did = 0;

		if (c == '/' && cn == '/') {
			// Simple coment
			ysen_pos_t* start = NULL;
			if (ysen_lex_getopt(YSEN_LEX_COMMENTS)) {
				start = ysen_pos_clone(l->pos);
			}
			ysen_inc(l); ysen_inc(l); // consume //
			while (!ysen_eof(l, 0) && ysen_peek(l, 0) != '\n') {
				ysen_inc(l); // Consume until newline
			}
			if (start) {
				ysen_token_t* tok = ysen_token_create();
				tok->type = ysen_token_comment;
				tok->source_range = ysen_range_adopt(start, ysen_pos_clone(l->pos));
				tok->content = YSEN_RSUBSTR(l->buf, tok->source_range);
				ysen_lex_append(l, tok);
			}
			ysen_inc(l); // consume n
			did = 1;
			printf("consumed simple comment, iter=%d\n", iter);
		}
		else if (c == '/' && cn == '*') {
			ysen_pos_t* start = NULL;
			if (ysen_lex_getopt(YSEN_LEX_COMMENTS)) {
				start = ysen_pos_clone(l->pos);
			}
			ysen_inc(l); ysen_inc(l); // consume /*
			while (!ysen_eof(l, 0)) {
				if (ysen_peek(l, 0) == '*' && ysen_peek(l, 1) == '/') {
					break;
				}

				ysen_inc(l);
			}

			ysen_inc(l); ysen_inc(l);
			if (start) {
				ysen_token_t* tok = ysen_token_create();
				tok->type = ysen_token_comment;
				tok->source_range = ysen_range_adopt(start, ysen_pos_clone(l->pos));
				tok->content = YSEN_RSUBSTR(l->buf, tok->source_range);
				ysen_lex_append(l, tok);
			}
			did = 1;
			printf("consumed complex comment, iter=%d\n", iter);
			printf("l->pos->ofs = %ld, len=%ld\n", l->pos->ofs, l->len);
		}

		if (!did) {
			break;
		}
		++iter;
	}
	ysen_consume_ws(l);
}

ysen_token_t* ysen_lex_impl(ysen_lexer_t* l)
{
	#define YSEN_LEXCHECK(x) if (!x) goto error
	#define YSEN_SINGLETOK(ch, str, t)								\
	   else if (c == ch) {											\
			ysen_token_t* tok = ysen_token_create();				\
			YSEN_LEXCHECK(tok);										\
			tok->type = t;											\
			tok->content = ysen_string_refstr(str);					\
			tok->source_range = ysen_range_with(l->pos, l->pos);	\
			ysen_lex_append(l, tok);								\
			ysen_inc(l);											\
	   } 


	while(!ysen_eof(l, 0)) {
		ysen_consume_comments(l);
		if (ysen_eof(l, 0)) break;

		char c = ysen_peek(l, 0);
		if (c == '\'') {
			// string
			ysen_token_t* tok = ysen_token_create();
			YSEN_LEXCHECK(tok);
			tok->type = ysen_token_string;
			tok->content = ysen_lexer_consume_str(l, &tok->source_range);
			YSEN_LEXCHECK(tok);
			ysen_lex_append(l, tok);
		}
		else if (isalpha(c)) {
			// identifier
			ysen_token_t* tok = ysen_token_create();
			YSEN_LEXCHECK(tok);
			tok->type = ysen_token_identifier;
			tok->content = ysen_lexer_consume_ident(l, &tok->source_range);
			YSEN_LEXCHECK(tok);
			ysen_lex_append(l, tok);
		}
		else if (isdigit(c) || c == '-' || c == '.') {
			// number
			ysen_token_t* tok = ysen_token_create();
			YSEN_LEXCHECK(tok);
			tok->type = ysen_token_identifier;
			tok->content = ysen_lexer_consume_digits(l, &tok->source_range, &tok->flags);
			YSEN_LEXCHECK(tok);
			ysen_lex_append(l, tok);
		}
		YSEN_SINGLETOK('.', ".", ysen_token_dot)	
		YSEN_SINGLETOK(',', ",", ysen_token_comma)	
		YSEN_SINGLETOK(':', ":", ysen_token_colon)	
		YSEN_SINGLETOK(';', ";", ysen_token_semicolon)	
		YSEN_SINGLETOK('[', "[", ysen_token_lbracket)	
		YSEN_SINGLETOK(']', "]", ysen_token_rbracket)	
		YSEN_SINGLETOK('{', "{", ysen_token_lsquiggly)	
		YSEN_SINGLETOK('}', "}", ysen_token_rsquiggly)	
		YSEN_SINGLETOK('(', "(", ysen_token_lparen)	
		YSEN_SINGLETOK(')', ")", ysen_token_rparen)	
		else if (isop(c)) {
			ysen_token_t* tok = ysen_token_create();
			YSEN_LEXCHECK(tok);
			tok->type = ysen_token_operator;
			tok->content = ysen_lexer_consume_op(l, &tok->source_range);
			printf("tok->content=%p, c=%c\n", tok->content, c);
			YSEN_LEXCHECK(tok);
			ysen_lex_append(l, tok);
		}
		else {
			fprintf(stderr, "Unknown c='%c', skip it\n", c);
			ysen_inc(l); // skip it
		}
	}
	
	return l->root_token;
error:
	printf("error\n");
	if (l->root_token) ysen_token_free(l->root_token);
	return NULL;
}




