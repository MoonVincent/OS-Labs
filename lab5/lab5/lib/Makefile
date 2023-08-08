# Makefile_lib
.PHONY: all
all: printk.o rand.o string.o

%.o: %.c
	@${GCC} ${CFLAG} -c $< -o $@

.PHONY: clean
clean:
	@rm *.o 

