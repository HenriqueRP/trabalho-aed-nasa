#include <limits.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reader_nasa.h"
#include "item_nasa.h"
#include "item.h"

// reader_nasa.c - CSV -> ITEM generico.
//
// Duas partes bem separadas: o parse do texto (dividir, sem_aspas, ler_*) e a
// montagem do ITEM. Se a Parte 2 usar outro dataset, este arquivo e o unico
// que precisa de um irmao - as estruturas nao mudam.
//
// Os contadores sao opacos: o struct contadores_leitura e definido apenas no
// reader.c. Aqui so existem os acessores e as funcoes de incremento.

#define NUM_CAMPOS 20    // o CSV da Parte 1 tem exatamente 20 colunas
#define TAM_LINHA  4096  // acima disso a linha e considerada truncada

// Verifica se fgets leu toda a linha; descarta os fragmentos excedentes.
static bool linha_completa(FILE *arquivo, const char *linha, size_t capacidade) {
    size_t tamanho = strlen(linha);
    if (tamanho < capacidade - 1 || strchr(linha, '\n') != NULL) return true;

    int proximo = fgetc(arquivo);
    if (proximo == EOF) return true; // linha no limite e sem quebra final
    while (proximo != '\n' && proximo != EOF) {
        proximo = fgetc(arquivo);
    }
    return false;
}

// Adaptadores: traduzem o ITEM_NASA para o contrato do ITEM generico.
// Ficam aqui (e nao no item.c) para manter o envelope cego ao dataset.
static void apagar_nasa(void **dado) {
    item_nasa_apagar((ITEM_NASA **)dado);
}

static void *copiar_nasa(const void *dado) {
    // item_nasa_copiar ainda recebe ponteiro nao-const, daí o cast
    return item_nasa_copiar((ITEM_NASA *)dado);
}

// Divide a linha por virgula respeitando aspas: uma virgula dentro de
// "...", como em "2020 AB1, fragmento B", nao separa campo.
// n comeca em 1 porque campos[0] e sempre o comeco da linha.
// Retorna 0 se houver mais de 20 campos ou se uma aspa nao fechar.
static size_t dividir(char *linha, char *campos[]) {
    size_t n = 0;
    bool dentro = false;
    campos[n++] = linha;

    for (char *p = linha; *p != '\0'; p++) {
        if (*p == '"') {
            if (dentro && *(p + 1) == '"') {
                p++;
            } else {
                dentro = !dentro;
            }
        } else if (*p == ',' && !dentro) {
            if (n == NUM_CAMPOS) return 0;
            *p = '\0';
            campos[n++] = p + 1;
        }
    }

    if (dentro) return 0;
    return n;
}

// Remove as aspas das pontas. O ITEM guarda o valor puro; quem devolve as
// aspas na saida e o writer, junto com o escape das aspas internas.
static char *sem_aspas(char *campo) {
    size_t n = strlen(campo);
    if (n >= 2 && campo[0] == '"' && campo[n - 1] == '"') {
        campo[n - 1] = '\0';
        return campo + 1;   // devolve o ponteiro ja deslocado
    }
    return campo;           // campo sem aspas: devolve como esta
}

// strtod com validacao completa. Celula vazia e rejeitada antes do parse:
// sem isso strtod("") devolveria 0 e o registro entraria com dado inventado.
static bool ler_double(const char *s, double *saida) {
    if (*s == '\0') return false;
    char *fim;
    errno = 0;
    double x = strtod(s, &fim);
    // rejeita cauda nao numerica ("12abc"), overflow e campo parcial
    if (fim == s || *fim != '\0' || errno == ERANGE || !isfinite(x)) return false;
    *saida = x;
    return true;
}

// Mesmo criterio do double, com strtol. A faixa de int e bem menor que a de
// long, entao o cast so acontece depois da validacao de fim de string.
static bool ler_int(const char *s, int *saida) {
    if (*s == '\0') return false;
    char *fim;
    errno = 0;
    long x = strtol(s, &fim, 10);
    if (fim == s || *fim != '\0' || errno == ERANGE ||
        x < INT_MIN || x > INT_MAX) return false;
    *saida = (int)x;
    return true;
}

// O CSV traz True/False capitalizado. Aceita as duas grafias para nao
// depender de como o arquivo foi gerado.
static bool ler_bool(const char *s, bool *saida) {
    if (!strcmp(s, "True")  || !strcmp(s, "true"))  { *saida = true;  return true; }
    if (!strcmp(s, "False") || !strcmp(s, "false")) { *saida = false; return true; }
    return false;   // celula vazia ou valor inesperado
}

bool reader_nasa_tem_campo(const char *campo) {
    // A tabela de campos vive no item_nasa.c: manter uma copia aqui faria
    // as duas divergirem na primeira mudanca de schema.
    return item_nasa_tem_campo(campo);
}

bool reader_nasa_loader(Estrutura *base, const Config *config,
                        ContadoresLeitura *contadores) {
    if (base == NULL || config == NULL || contadores == NULL) return false;
    
    FILE *arquivo = fopen(config_get_input(config), "r");
    if (arquivo == NULL) {
        perror("Reader: nao foi possivel abrir o CSV");
        return false;
    }

    char linha[TAM_LINHA];
    char *campos[NUM_CAMPOS];

    // A primeira linha e o cabecalho: descartada aqui e recopiada do proprio
    // input pelo writer, o que garante que a saida tem o mesmo cabecalho.
    if (fgets(linha, sizeof linha, arquivo) == NULL) {
        fclose(arquivo);
        return false;   // arquivo vazio ou ilegivel
    }
    if (!linha_completa(arquivo, linha, sizeof linha)) {
        fclose(arquivo);
        return false;   // cabecalho truncado
    }

    while (fgets(linha, sizeof linha, arquivo) != NULL) {
        // -limit corta por registros INSERIDOS, nao por linhas lidas: uma
        // linha invalida nao consome cota.
        if (config_get_limit(config) > 0 &&
            contadores_inseridos(contadores) >= (size_t)config_get_limit(config))
            break;

        contadores_inc_input(contadores);
        if (!linha_completa(arquivo, linha, sizeof linha)) {
            contadores_inc_invalidos(contadores);
            continue;
        }
        linha[strcspn(linha, "\r\n")] = '\0';   // tolera final CRLF

        // Layout inesperado: conta como invalida e segue para a proxima linha.
        // Uma linha ruim nao pode derrubar a carga inteira.
        if (dividir(linha, campos) != NUM_CAMPOS) {
            contadores_inc_invalidos(contadores);
            continue;
        }
        for (size_t i = 0; i < NUM_CAMPOS; i++) campos[i] = sem_aspas(campos[i]);

        int ano, mes, dias, nivel;
        double dist, vel, mag, risco;

        // Campos obrigatorios, todos numericos e todos no mesmo if: se
        // qualquer um falhar, a linha inteira e descartada.
        if (!ler_int(campos[3], &ano)     || !ler_int(campos[4], &mes) ||
            !ler_double(campos[5], &dist) || !ler_double(campos[6], &vel) ||
            !ler_double(campos[7], &mag)  || !ler_int(campos[8], &dias) ||
            !ler_double(campos[11], &risco) || !ler_int(campos[12], &nivel)) {
            contadores_inc_invalidos(contadores);
            continue;
        }

        // Sentry e opcional no dataset: comeca em NAN, que significa "ausente".
        // Zero seria um valor legitimo e passaria em filtro numerico como se
        // fosse medicao real. Por isso NAN e nao 0.0.
        double impacto = NAN, torino = NAN, palermo = NAN, diametro = NAN;

        bool passado = false, futuro = false, sentry = false;
        // O retorno do parse vira a flag de validade do booleano: e assim que
        // "celula vazia" fica distinguivel de "False".
        bool passado_ok = ler_bool(campos[9],  &passado);
        bool futuro_ok  = ler_bool(campos[10], &futuro);
        bool sentry_ok  = ler_bool(campos[15], &sentry);

        // Aqui a ausencia e aceitavel, entao o retorno e ignorado de proposito:
        // campo vazio mantem o NAN inicial.
        ler_double(campos[16], &impacto);
        ler_double(campos[17], &torino);
        ler_double(campos[18], &palermo);
        ler_double(campos[19], &diametro);

        ITEM_NASA *nasa = item_nasa_criar(
            campos[0], campos[1], campos[2],
            ano, mes,
            dist, vel, mag, dias,
            passado, passado_ok,
            futuro, futuro_ok,
            risco, nivel,
            campos[13], campos[14],
            sentry, sentry_ok,
            impacto, torino, palermo, diametro
        );

        // Envelope generico. A chave e o numero da linha: desempate estavel
        // no top-k quando duas notas de similaridade empatam.
        ITEM *item = NULL;
        if (nasa != NULL) {
            item = item_criar(nasa, apagar_nasa, copiar_nasa,
                              item_nasa_ler_campo,
                              (int)contadores_input(contadores));
            if (item == NULL) item_nasa_apagar(&nasa);   // envelope falhou
        }

        // Uma saida de erro para os dois casos: item NULL (falta de memoria)
        // e recusa da estrutura. item_apagar aceita NULL, entao nao precisa
        // de teste separado.
        if (item == NULL || !estrutura_inserir(base, item)) {
            item_apagar(&item);
            contadores_inc_invalidos(contadores);
            continue;
        }
        contadores_inc_inseridos(contadores);
    }

    if (ferror(arquivo)) {   // erro de leitura no meio do arquivo
        fclose(arquivo);
        return false;
    }
    fclose(arquivo);
    return true;
}
