
MATH=math_cgi

MATH_SRC=$(shell ls | grep -E "*.c")
MATH_OBJ=$(MATH_SRC:.c=.o)

.PHONY:all
all:$(MATH)

$(MATH):math_cgi.c
	gcc -o $@ $^

%.o:$(shell pwd)/%.c
	gcc -c $<

.PHONY:clean
clean:
	rm -f  math_cgi
