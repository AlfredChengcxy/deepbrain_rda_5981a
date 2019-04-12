TARGET = test

SRC_FILES := $(wildcard *.c)

OBJ_FILES := $(SRC_FILES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	gcc -o $@ $^

%.o: %.c
	gcc -I. -o $@ -c $<

clean:
	rm -f *.o
	rm -f $(TARGET)
