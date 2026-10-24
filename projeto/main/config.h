#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// as oito estruturas que podem vir em --base e --result
typedef enum opcao_estrutura {
pilha,
fila,
lista_seq,
lista_enc,
lista_cabeca,
lista_ord,
lista_gen,
lista_cruz
} opcao_estrutura;

// tudo que veio da linha de comando fica aqui
// os quatro caminhos são alocados no config_init, então precisam de free
typedef struct {
char *reader; // nome do leitor de dataset
char *input; // csv de entrada
char *path; // arquivo txt da consulta
char *output; // csv de saída
opcao_estrutura base; // estrutura que guarda o dataset
opcao_estrutura resultado; // estrutura que guarda o resultado
int top_k; // 0 = todos
int limit; // 0 = todos
} Config;

// lê argv e preenche o config. false = deu erro (o que já alocou é liberado)
bool config_init(Config *config, char **argv);

// libera os quatro caminhos e zera os ponteiros. seguro chamar duas vezes
void config_apagar(Config *config);

#endif