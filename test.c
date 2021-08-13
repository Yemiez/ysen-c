#include "ysen/ysen.h"
#include <stdio.h>
#include <stdlib.h>

char* read_file(const char* filename)
{
	FILE* f = fopen(filename, "r");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* buf = malloc(len + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	buf[len] = 0;

	if (!fread(buf, sizeof(char), len, f)) {
		fclose(f);
		free(buf);
		return NULL;
	}

	fclose(f);
	return buf;
}

int main()
{
	ysen_lex_setopt(YSEN_LEX_COMMENTS, 1);

	char* content = read_file("test.ysen");
	if (!content) {
		fprintf(stderr, "Could not read file");
		return -1;
	}

	ysen_token_t* root = ysen_lex(content);

	ysen_token_t* tok = root;
	while (tok != NULL) {
		printf("tok: %p, type=%s", tok, ysen_token_type_string(tok));
		printf(", str=%s\n", tok->content ? tok->content->buf : "null");
	
		tok = tok->next;
	}
	ysen_token_free(tok);
}
