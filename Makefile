CFLAGS ?= -Wall -Werror
CFLAGS += -Iinclude
LDFLAGS ?=

PROG := su-exec
SRCS := $(PROG).c
INCS := license.inc

PREFIX := /usr/local
INSTALL_DIR := $(PREFIX)/bin
MAN_DIR := $(PREFIX)/share/man/man8

all: $(PROG)

license.inc: LICENSE
	xxd -i $^ > $@

$(PROG): $(SRCS) $(INCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

$(PROG)-static: $(SRCS) $(INCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) -static $(LDFLAGS)

$(PROG)-debug: $(SRCS) $(INCS)
	$(CC) -g $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

install:
	install -Dm755 $(PROG) $(DESTDIR)$(INSTALL_DIR)/$(PROG)
	install -Dm644 $(PROG).1 $(DESTDIR)$(MAN_DIR)/$(PROG).1

clean:
	rm -f $(PROG) $(PROG)-static $(PROG)-debug $(INCS)

