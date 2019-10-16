define define_c_target
$(1)_src_files:=$(2)
$(1)_obj_files:=$$($(1)_src_files:.c=.o)

$$($(1)_obj_files): %.o: %.c
	@echo "\tCC\t$$@"
	$$(CC) -c $$< -o $$@ $$($(1)_cflags)

$(1): $$($(1)_obj_files)
	@echo "\tLD\t$$@"
	$$(CC) $$^ -o $$@ $$($(1)_ldflags)

$(1)_clean:
	@echo "\tCLEAN\t$(1)"
	rm -f $$($(1)_obj_files) $(1)
endef