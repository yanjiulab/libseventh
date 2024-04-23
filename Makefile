EXAMPLE_DIR := ./examples
LIB_DIR := ./lib

EXAMPLE_SRCS=$(shell find $(EXAMPLE_DIR) -name '*.c')
EXAMPLES = $(notdir $(patsubst %.c, %, $(EXAMPLE_SRCS)))

EV_H = ./src/seventh.h
EV_C = ./src/seventh.c

CFLAGS = -g3 -O2 -Wno-sign-compare  -I./src -DEVENT_EPOLL

.PHONY: all

all:
	$(CC) -c ${EV_C} -o ${LIB_DIR}/seventh.o
	ar rc ${LIB_DIR}/libseventh.a ${LIB_DIR}/seventh.o
	$(CC) $(CPPFLAGS) $(CFLAGS) ${EV_C} -fPIC -shared -o $(LIB_DIR)/libseventh.so

example:
	@echo $(EXAMPLES)
	@for target in $(EXAMPLES); \
	do					\
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXAMPLE_DIR)/$$target.c ${EV_H} ${EV_C} -o $(EXAMPLE_DIR)/$$target $(LDFLAGS); \
	done

install:
	@cp -v ${LIB_DIR}/libseventh.a /usr/local/lib/
	@cp -v ${LIB_DIR}/libseventh.so /usr/local/lib/
	@cp -v ${EV_H} /usr/local/include/

uninstall:
	@rm -v /usr/local/lib/libseventh.a
	@rm -v /usr/local/lib/libseventh.so
	@rm -v /usr/local/include/seventh.h

clean:
	@for target in $(EXAMPLES); \
	do \
	$(RM) $(EXAMPLE_DIR)/$$target; \
	done