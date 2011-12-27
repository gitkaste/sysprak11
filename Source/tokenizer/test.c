#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "tokenizer.h"

#define NOMEM if(1){fprintf(stderr,"ALARM: Malloc failed"); exit(1);}

int main(){
	char * testarr[] = { "much", "cc", "c", "tork", "maehmuh","tokenmaeh","muhtokenmaehnub", "cmuhmork", "computer", "", NULL }; 
	int tokenstart, tokenlength, fulllength, i=0;
	struct buffer token;
	char * temp;
	while ( testarr[i]!=NULL ){
		temp = malloc(strlen(testarr[i]) +1);
		if ( temp  == NULL ){ NOMEM }
		strcpy(temp, testarr[i]);
		struct buffer buf = {(unsigned char *)temp, strlen(testarr[i])+1, strlen(testarr[i])};
		int ret = vaToken( &buf, &tokenstart, &tokenlength, &fulllength, "maeh", "muh", "c", "comp", NULL);
		token.buf = malloc(tokenlength+1);
	 	token.bufmax = tokenlength+1;
		token.buflen = 0;
		if (token.buf) {
			printf("%d | tokenstart %u tokenlength %u fulllength %u %s \n", ret, tokenstart, tokenlength, fulllength, buf.buf);
			ret = extractToken(&buf, &token, tokenstart, tokenlength, fulllength);
			printf("%d : buffer '%s' token '%s' \n", ret, buf.buf, token.buf);
		} else{ NOMEM }
		i++;
		free(temp);
	}
	return 0;
}
