CC      := gcc
CFLAGS  := -Wall -Wextra -g
LDLIBS  := -lreadline

TARGET  := cykor2

SRCS    := Cykor2.c

OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)