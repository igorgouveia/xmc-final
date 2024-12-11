CC = gcc
CFLAGS = -Wall -O2

# Detecta o sistema operacional
UNAME_S := $(shell uname -s)

# Configurações específicas para MacOS
ifeq ($(UNAME_S),Darwin)
    GLPK_CFLAGS = -I/opt/homebrew/include
    GLPK_LIBS = -L/opt/homebrew/lib -lglpk
else
    # Para Linux e outros, usa pkg-config
    GLPK_CFLAGS = $(shell pkg-config --cflags glpk)
    GLPK_LIBS = $(shell pkg-config --libs glpk)
endif

all: tsp_bb tsp_mip

tsp_bb: src/main.c src/tsp_bb.c src/tsp_common.c
	$(CC) $(CFLAGS) -DUSE_BB -o tsp_bb src/main.c src/tsp_bb.c src/tsp_common.c

tsp_mip: src/main.c src/tsp_mip.c src/tsp_common.c
	$(CC) $(CFLAGS) $(GLPK_CFLAGS) -o tsp_mip src/main.c src/tsp_mip.c src/tsp_common.c $(GLPK_LIBS)

clean:
	rm -f tsp_bb tsp_mip *.o