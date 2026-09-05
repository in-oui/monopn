CC ?= cc
CPPFLAGS += -DDEV -I. '-D__RCSID(x)=' '-D__COPYRIGHT(x)='
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Wno-old-style-definition \
	-Wno-missing-field-initializers

PROG = monopn
CARD_FILE = cards.pck
CARD_BUILDER = initdeck

SRCS = monop.c cards.c execute.c getinp.c houses.c jail.c malloc.c misc.c \
	morg.c print.c prop.c rent.c roll.c spec.c trade.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(PROG) $(CARD_FILE)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(CARD_BUILDER): initdeck.c deck.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ initdeck.c

$(CARD_FILE): $(CARD_BUILDER) cards.inp
	./$(CARD_BUILDER) cards.inp $@

%.o: %.c monop.h monop.ext deck.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PROG) $(CARD_BUILDER) $(CARD_FILE) $(OBJS)

