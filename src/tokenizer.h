#ifndef _tokenizer_h
#define _tokenizer_h

#include "util.h"
#include <stdarg.h>


/* getToken*-Functions
 * All these functions attempt to extract a token from the buffer.
*/

/* getTokenFromStream
 * This function blocks (and is therefore _not_ useable in conjunction with
 * polling multiple file-descriptors) until a token arrives on fd. It shall be
 * used in simple cases (the process doesn't handle signals) like reading the
 * config file or receiving filelists and results.
 * Return Values are: -1 on error, 0 on stream-end, 1 on token found
*/
int getTokenFromStream(int fd, struct buffer *buf, struct buffer *token, ...);

/* getTokenFromStreamBuffer
 * This function is usable in multiple fd (poll) cases. It doesn't read from
 * a filedescriptor, but expects a buffer that contains just the data that
 * arrived from the stream (and is therefore not terminated by anything).
 * Return Values are: -1 on error, 0 on no token, 1 on token found
*/
int getTokenFromStreamBuffer(struct buffer *buf, struct buffer *token, ...);

/* getTokenFromBuffer
 * This function is used for further slicing down contents of a buffer. For
 * example making "words" out of "lines" (lines were already tokenized by
 * getTokenFromStreamBuffer). The buffer used here is already null-terminated.
 * Return Values are: -1 on error, 0 on no token, 1 on token found
*/
int getTokenFromBuffer(struct buffer *buf, struct buffer *token, ...);


/* searchString
 * searchString uses tokenizer functionality (searchToken) to determine if any
 * of multiple search-strings are in char *string. It doesn't alter anything.
 * Return Values: 1 search-string found, 0 nothing found.
*/
int searchString(char *string, ...);

/* searchToken 
 * This function searches for the first non keyword tokenstart in buffer_temp
 * the keywords ap delimit the tokens
*/
int searchToken(int *tokenstart,int *tokenlength,int *fulllength, struct buffer *buffer_temp, va_list ap);

/* vaToken is just a debug function */
int vaToken(struct buffer *buffer_temp, int *tokenstart, int *tokenlength, int *fulllength, ...);

/* extractToken
 * ... extracts token of tokenlength from buffer_temp starting at tokenstart */
int extractToken(struct buffer *buffer_temp, struct buffer *token, int tokenstart,int tokenlength, int fulllength);
#endif
