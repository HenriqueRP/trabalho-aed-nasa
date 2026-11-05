#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

//as oito estruturas que podem vir em --base e --result
typedef enum opcao_estrutura {
    pilha,
    fila,
    lista_seq,
    lista_enc,
    lista_cabeca,
    lista_ord,
    lista_gen,
    lista_cruz,
    invalid = -1
} OpcaoEstrutura;

typedef struct config_ Config;

//valida argv, cria a configuracao e retorna NULL se houver erro.
Config *config_criar(int argc, char **argv);

// Os caminhos apontam para argv e nao devem ser liberados pelo programa.
const char *config_get_reader(const Config *config);
const char *config_get_input(const Config *config);
const char *config_get_path(const Config *config);
const char *config_get_output(const Config *config);
OpcaoEstrutura config_get_base(const Config *config);
OpcaoEstrutura config_get_resultado(const Config *config);
int config_get_top_k(const Config *config);
int config_get_limit(const Config *config);

// Libera apenas a configuracao; os caminhos pertencem a argv.
void config_apagar(Config **config);

#endif
