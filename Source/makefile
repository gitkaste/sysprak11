clean test: 
	$(MAKE) -C util $@
	$(MAKE) -C tokenizer $@
	$(MAKE) -C logger $@
	$(MAKE) -C directoryParser $@
	$(MAKE) -C server $@
	$(MAKE) -C client $@
	$(MAKE) -C protocol $@
	$(MAKE) -C consoler $@

server client: 
	$(MAKE) -C $@ $@
	cp $@/$@  .
