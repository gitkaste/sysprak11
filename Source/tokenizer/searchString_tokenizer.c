int searchString(char *string, ...) {
	va_list ap;
	int t;
	int ts, tl, fl;
    
	struct buffer str;

	str.buf = string;
	str.buflen = strlen(string);
	str.bufmax = str.buflen + 1;
	
	
	va_start(ap, string);
	t = searchToken(&ts, &tl, &fl, &str, ap);
	va_end(ap);
	
	/* if ts != 0 there was a separator (search-string) at the beginning
	 * and we found something */
	if(t == 0 && ts != 0) t = 1;
	
	return t;
}
