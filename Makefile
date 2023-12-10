CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -pedantic -O3
AR = ar
ARFLAGS = rcs
RANLIB = ranlib
SRC=src
INC=include
LIB_OBJ=lib
UGP_DEMO=ugp-demo

all: libptrace_do.a migrate.a ugp-demo

libptrace_do.a: $(SRC)/libptrace_do.c $(SRC)/parse_maps.c
	mkdir -p $(LIB_OBJ)
	$(CC) $(CFLAGS) -I$(INC) -o $(LIB_OBJ)/parse_maps.o -c $(SRC)/parse_maps.c
	$(CC) $(CFLAGS) -I$(INC) -o $(LIB_OBJ)/libptrace_do.o -c $(SRC)/libptrace_do.c
	$(AR) $(ARFLAGS) $(LIB_OBJ)/libptrace_do.a $(LIB_OBJ)/libptrace_do.o $(LIB_OBJ)/parse_maps.o
	$(RANLIB) $(LIB_OBJ)/libptrace_do.a

migrate.a: libptrace_do.a $(SRC)/migrate.c
	$(CC) $(CFLAGS) -I$(INC) -o $(LIB_OBJ)/migrate.o -c $(SRC)/migrate.c 
	$(AR) $(ARFLAGS) $(LIB_OBJ)/migrate.a $(LIB_OBJ)/libptrace_do.o $(LIB_OBJ)/migrate.o $(LIB_OBJ)/parse_maps.o
	$(RANLIB) $(LIB_OBJ)/migrate.a

# clone: $(SRC)/clone.c
# 	$(CC) $(CFLAGS) -I$(INC) -o $(SRC)/clone $(SRC)/clone.c $(LIB_OBJ)/libptrace_do.a

ugp-demo: manager pstree_setup

manager: $(UGP_DEMO)/manager.c
	$(CC) $(CFLAGS) -I$(INC) -o $(UGP_DEMO)/manager $(UGP_DEMO)/manager.c $(LIB_OBJ)/migrate.a

pstree_setup: $(UGP_DEMO)/pstree_setup.c
	$(CC) $(CFLAGS) -I$(INC) -o $(UGP_DEMO)/pstree_setup $(UGP_DEMO)/pstree_setup.c

clean: 
	rm -rf $(LIB_OBJ)
	rm $(UGP_DEMO)/manager
	rm $(UGP_DEMO)/pstree_setup
