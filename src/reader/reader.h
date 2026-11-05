#ifndef READER_H
#define READER_H

#include <stdbool.h>
#include <stddef.h>

#include "estrutura.h"
#include "config.h"

// reader.h - fachada opaca dos readers de CSV.
//
// Nada do dataset vaza por aqui: nem o enum de readers, nem os campos dos
// contadores. Quem chama ve apenas reader_load, reader_tem_campo e as
// funcoes de contadores abaixo.
//
// ContadoresLeitura e um tipo INCOMPLETO de proposito. A definicao do struct
// fica dentro do reader.c, e em nenhum outro lugar. Sem a definicao, ninguem
// le nem escreve os campos de fora - so pelos acessores.

typedef struct contadores_leitura ContadoresLeitura;

// Tipo incompleto significa que uma instancia so pode existir no heap:
// fora do reader.c o compilador nao sabe o sizeof.
ContadoresLeitura *contadores_criar(void);
void contadores_apagar(ContadoresLeitura **contadores);

// Zera os tres contadores. Chamado no inicio da carga para o relatorio nunca
// herdar numeros de uma execucao anterior.
void contadores_resetar(ContadoresLeitura *contadores);

// Leitura. Devolvem 0 quando o ponteiro e NULL.
size_t contadores_input(const ContadoresLeitura *contadores);
size_t contadores_inseridos(const ContadoresLeitura *contadores);
size_t contadores_invalidos(const ContadoresLeitura *contadores);

// Escrita. Os loaders nao veem o struct, entao cada incremento passa por uma
// destas funcoes em vez de um "contadores->campo++".
void contadores_inc_input(ContadoresLeitura *contadores);
void contadores_inc_inseridos(ContadoresLeitura *contadores);
void contadores_inc_invalidos(ContadoresLeitura *contadores);

// Carga completa: resolve o dataset a partir de config_get_reader, abre o
// CSV de config_get_input, valida e insere em base.
bool reader_load(Estrutura *base, const Config *config,
                 ContadoresLeitura *contadores);

// O campo existe no dataset configurado? Resolve o reader por dentro, para o
// main validar a consulta sem conhecer o enum.
bool reader_tem_campo(const Config *config, const char *campo);

#endif