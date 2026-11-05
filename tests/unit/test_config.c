#include "test_util.h"
#include "config.h"

static Config *com(int argc, char **argv) { return config_criar(argc, argv); }

static void argumentos_validos(void) {
    Config *c = teste_config("nasa", "in.csv", "lista_enc", "lista_ord",
                             "q.txt", "out.csv", 7, 100);
    CHECK(c != NULL);
    CHECK_STR(config_get_reader(c), "nasa");
    CHECK_STR(config_get_input(c), "in.csv");
    CHECK_STR(config_get_path(c), "q.txt");
    CHECK_STR(config_get_output(c), "out.csv");
    CHECK_EQ_INT(config_get_base(c), lista_enc);
    CHECK_EQ_INT(config_get_resultado(c), lista_ord);
    CHECK_EQ_INT(config_get_top_k(c), 7);
    CHECK_EQ_INT(config_get_limit(c), 100);
    config_apagar(&c);
    CHECK(c == NULL);
}

static void ordem_dos_pares_nao_importa(void) {
    char *argv[] = {"aed", "--limit", "0", "--top-k", "3", "--output", "o.csv",
                    "--query", "q.txt", "--result", "fila", "--base", "pilha",
                    "--input", "i.csv", "--reader", "ancine"};
    Config *c = com(17, argv);
    CHECK(c != NULL);
    CHECK_EQ_INT(config_get_base(c), pilha);
    CHECK_EQ_INT(config_get_resultado(c), fila);
    CHECK_STR(config_get_reader(c), "ancine");
    config_apagar(&c);
}

static void todas_as_oito_estruturas(void) {
    const char *n[] = {"pilha", "fila", "lista_seq", "lista_enc",
                       "lista_cabeca", "lista_ord", "lista_gen", "lista_cruz"};
    OpcaoEstrutura e[] = {pilha, fila, lista_seq, lista_enc,
                          lista_cabeca, lista_ord, lista_gen, lista_cruz};
    for (int i = 0; i < 8; i++) {
        Config *c = teste_config("nasa", "i", n[i], "fila", "q", "o", 0, 0);
        CHECK(c != NULL);
        if (c) CHECK_EQ_INT(config_get_base(c), e[i]);
        config_apagar(&c);
    }
}

static void argumentos_invalidos(void) {
    CHECK(teste_config("nasa", "i", "arvore", "fila", "q", "o", 0, 0) == NULL); // estrutura desconhecida
    CHECK(teste_config("nasa", "i", "fila", "heap", "q", "o", 0, 0) == NULL);

    char *poucos[] = {"aed", "--reader", "nasa"};
    CHECK(com(3, poucos) == NULL);
    CHECK(com(1, (char *[]){"aed"}) == NULL);

    char *repetido[] = {"aed", "--reader", "nasa", "--reader", "nasa", "--base", "fila",
                        "--result", "fila", "--query", "q", "--output", "o",
                        "--top-k", "1", "--limit", "1"};
    CHECK(com(17, repetido) == NULL); // --input ausente, --reader duplicado

    char *desconhecida[] = {"aed", "--reader", "nasa", "--input", "i", "--base", "fila",
                            "--result", "fila", "--query", "q", "--output", "o",
                            "--top-k", "1", "--limite", "1"};
    CHECK(com(17, desconhecida) == NULL);

    char *negativo[] = {"aed", "--reader", "nasa", "--input", "i", "--base", "fila",
                        "--result", "fila", "--query", "q", "--output", "o",
                        "--top-k", "-1", "--limit", "1"};
    CHECK(com(17, negativo) == NULL);

    char *texto[] = {"aed", "--reader", "nasa", "--input", "i", "--base", "fila",
                     "--result", "fila", "--query", "q", "--output", "o",
                     "--top-k", "3", "--limit", "abc"};
    CHECK(com(17, texto) == NULL);

    char *cauda[] = {"aed", "--reader", "nasa", "--input", "i", "--base", "fila",
                     "--result", "fila", "--query", "q", "--output", "o",
                     "--top-k", "3x", "--limit", "1"};
    CHECK(com(17, cauda) == NULL);
    CHECK(com(17, NULL) == NULL);
}

static void getters_com_ponteiro_nulo(void) {
    CHECK(config_get_reader(NULL) == NULL);
    CHECK(config_get_input(NULL) == NULL);
    CHECK_EQ_INT(config_get_base(NULL), invalid);
    CHECK_EQ_INT(config_get_top_k(NULL), 0);
    config_apagar(NULL);
}

int main(void) {
    printf("test_config\n");
    RUN(argumentos_validos);
    RUN(ordem_dos_pares_nao_importa);
    RUN(todas_as_oito_estruturas);
    RUN(argumentos_invalidos);
    RUN(getters_com_ponteiro_nulo);
    return testes_resumo("test_config");
}