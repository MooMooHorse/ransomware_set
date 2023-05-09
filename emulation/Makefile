# tool macros
C := gcc
CFLAGS := -pthread -g
SRC := $(wildcard rte_ring/*.c) $(wildcard *.c)
EXE := main

CLEAN_LIST := $(wildcard *.o) $(EXE)

.PHONY: all
all: 
	$(C) $(CFLAGS) $(SRC) -o $(EXE) 


.PHONY: debug
debug: $(TARGET_DEBUG)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

.PHONY: distclean
distclean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(DISTCLEAN_LIST)
