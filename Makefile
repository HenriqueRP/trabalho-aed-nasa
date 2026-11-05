# Makefile - trabalho de AED (pipeline reader -> base -> search -> result -> writer)
#
# Alvos principais:
#   make            compila bin/aed
#   make test       compila e roda testes unitarios e de integracao
#   make run        roda o pipeline sobre os dados de exemplo (data/)
#   make valgrind   roda a suite de testes sob valgrind (se instalado)
#   make clean      remove build/ e bin/
#
# Requer GNU make e um compilador C11 (gcc/clang). No Windows use MSYS2/WSL.

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
LDLIBS  := -lm

# Cada diretorio com .h/.c em src/ entra no -I (os includes sao por nome).
SRC_DIRS := $(wildcard src/*/) $(wildcard src/eds/*/)
INCLUDES := $(addprefix -I,$(SRC_DIRS))

SRCS     := $(wildcard $(addsuffix *.c,$(SRC_DIRS)))
LIB_SRCS := $(filter-out src/main/main.c,$(SRCS))
LIB_OBJS := $(patsubst src/%.c,build/obj/%.o,$(LIB_SRCS))
MAIN_OBJ := build/obj/main/main.o

BIN      := bin/aed
LIB      := build/libaed.a

UNIT_SRCS := $(wildcard tests/unit/test_*.c)
UNIT_BINS := $(patsubst tests/unit/%.c,build/tests/%,$(UNIT_SRCS))
TEST_UTIL := build/obj/tests/test_util.o
PIPE_TEST := build/tests/test_pipeline

.PHONY: all test unit integration run run-detalhado valgrind clean help

all: $(BIN)

help:
	@sed -n '2,12p' Makefile

$(BIN): $(MAIN_OBJ) $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(LIB): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

build/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# ------------------------------------------------------------------ testes

$(TEST_UTIL): tests/test_util.c tests/test_util.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -Itests -c $< -o $@

build/tests/%: tests/unit/%.c $(TEST_UTIL) $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -Itests $< $(TEST_UTIL) $(LIB) $(LDLIBS) -o $@

$(PIPE_TEST): tests/integration/test_pipeline.c $(TEST_UTIL) $(LIB) $(BIN)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -Itests $< $(TEST_UTIL) $(LIB) $(LDLIBS) -o $@

unit: $(UNIT_BINS)
	@fail=0; for t in $(UNIT_BINS); do ./$$t || fail=1; done; exit $$fail

integration: $(PIPE_TEST)
	./$(PIPE_TEST) $(BIN) data

test: unit integration
	@echo "Todos os testes passaram."

# ---------------------------------------------------------------- execucao

# Exemplo: base e resultado em estruturas diferentes sobre a amostra NASA.
run: $(BIN)
	@mkdir -p build
	$(BIN) --reader nasa --input data/nasa_amostra.csv \
	       --base lista_enc --result lista_ord \
	       --query data/consulta_nasa.txt --output build/saida_nasa.csv \
	       --top-k 5 --limit 0

run-detalhado: $(BIN)
	@mkdir -p build
	AED_REPORT_DETALHADO=1 $(MAKE) --no-print-directory run

valgrind: $(UNIT_BINS) $(PIPE_TEST)
	@for t in $(UNIT_BINS); do \
	  valgrind --error-exitcode=1 --leak-check=full -q ./$$t || exit 1; done
	@echo "Sem erros de memoria nos testes unitarios."

clean:
	rm -rf build bin

-include $(LIB_OBJS:.o=.d) $(MAIN_OBJ:.o=.d)
