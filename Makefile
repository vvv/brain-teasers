## ---------------------------------------------------------------------
## Configurable section

CPPFLAGS = -DDEBUG

CFLAGS = -g -Wall -Wextra
# CFLAGS = -O3 -Wall -Wextra

# LDFLAGS =
# LDLIBS =

PROG = task1
SRC = btree.c main.c

## ---------------------------------------------------------------------
## The stuff below is not supposed to be touched frequently

SHELL = /bin/sh

all: $(PROG)

OBJ := $(SRC:.c=.o)
-include $(OBJ:.o=.d)

# Generate dependencies
%.d: %.c
	cpp -MM $(CPPFLAGS) $< |\
 sed -r 's%^(.+)\.o:%$(@D)/\1.d $(@D)/\1.o:%' >$@

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

mostlyclean:
	rm -f $(OBJ) $(OBJ:.o=.d)

clean: mostlyclean
	rm -f $(PROG)

.PHONY: mostlyclean clean
