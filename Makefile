
FONTES = \
	src/main/main.c \
	src/main/config.c \
	src/main/report.c \
	src/reader/reader.c \
	src/reader/reader_nasa.c \
	src/reader/reader_ancine.c \
	src/query/query.c \
	src/search/search.c \
	src/writer/writer.c \
	src/eds/estrutura/estrutura.c \
	src/eds/tad_item/item.c \
	src/eds/tad_item_nasa/item_nasa.c \
	src/eds/tad_item_ancine/item_ancine.c \
	src/eds/pilha/pilha.c \
	src/eds/fila/fila.c \
	src/eds/lista_sequencial/lista_sequencial.c \
	src/eds/lista_encadeada/lista_encadeada.c \
	src/eds/lista_cabeca/lista_cabeca.c \
	src/eds/lista_ordenada/lista_ordenada.c \
	src/eds/lista_generalizada/lista_generalizada.c \
	src/eds/listas_cruzada/lista_cruzada.c

INCLUDES = \
	-Isrc/main \
	-Isrc/reader \
	-Isrc/query \
	-Isrc/search \
	-Isrc/writer \
	-Isrc/eds/estrutura \
	-Isrc/eds/tad_item \
	-Isrc/eds/tad_item_nasa \
	-Isrc/eds/tad_item_ancine \
	-Isrc/eds/pilha \
	-Isrc/eds/fila \
	-Isrc/eds/lista_sequencial \
	-Isrc/eds/lista_encadeada \
	-Isrc/eds/lista_cabeca \
	-Isrc/eds/lista_ordenada \
	-Isrc/eds/lista_generalizada \
	-Isrc/eds/listas_cruzada

all: trabalho

trabalho: $(FONTES)
	gcc $(FONTES) $(INCLUDES) -o trabalho -std=c99 -Wall -Wextra -pedantic -lm

clean:
	rm -f trabalho
