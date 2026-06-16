CC      = cc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS =

TARGET  = nvidia-power-governor
TEST    = test_runner

SRCS    = main.c config.c gpu.c regulate.c
OBJS    = $(SRCS:.c=.o)

PREFIX  = /usr/local

.PHONY: all clean test install static

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

static: LDFLAGS += -static
static: $(TARGET)

test: $(TEST)
	./$(TEST)

$(TEST): test.o config.o regulate.o
	$(CC) -o $@ $^

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/bin/$(TARGET)

clean:
	rm -f main.o config.o gpu.o regulate.o test.o $(TARGET) $(TEST)
