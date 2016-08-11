ROOT_PATH=$(shell pwd)

MATH_PATH=$(ROOT_PATH)/htdoc/math_cgi
MATH=math_cgi
MATH_SRC=$(shell ls $(MATH_PATH) | grep -E "*.c")
MATH_OBJ=$(MATH_SRC:.c=.o)

BIN=httpd
SRC=httpd.c
OBJ=$(SRC:.c=.o)
CC=gcc 
LDFLAGS=-l pthread -w
FLAGS=-D _DEBUG2_

.PHONY:all
all:$(BIN) $(MATH)	
$(BIN):$(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 
%.o:%.c
	$(CC) -c $<	$(LDFLAGS) $(FLAGS)

$(MATH):$(MATH_OBJ)
	$(CC) -o $@ $^
%.o:$(MATH_PATH)/%.c
	$(CC) -c $<

.PHONY:clean
clean:
	rm -f $(OBJ) $(BIN) $(MATH) *.o
	rm -rf output

.PHONY:output
output:$(BIN)
	@mkdir -p           output/http
	@cp -rf conf        output/http
	@cp -rf $(BIN)      output/
	@cp -rf start.sh    output/
	@mkdir -p           output/htdoc
	@mkdir -p           output/htdoc/cgi
		
	@cp -rf $(ROOT_PATH)/htdoc/ali.html   	output/htdoc
	@cp -rf $(ROOT_PATH)/math_cgi           output/htdoc/cgi
	@cp -rf $(ROOT_PATH)/htdoc/image        output/htdoc/image
	
.PHONY:debug
debug:
	@echo $(SRC)
	@echo $(OBJ)

