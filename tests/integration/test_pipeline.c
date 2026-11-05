// Teste do pipeline completo: executa o binario real (reader -> BASE ->
// search -> RESULT -> writer -> report) e confere a saida em stdout e o CSV.
//
// Uso: test_pipeline <binario> <pasta_dados>
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_util.h"

static const char *BIN;
static const char *DADOS;

static const char *ESTRUTURAS[] = {
    "pilha", "fila", "lista_seq", "lista_enc",
    "lista_cabeca", "lista_ord", "lista_gen", "lista_cruz"
};
#define N_EST 8

typedef struct { int status; char saida[4096]; } Execucao;

// Roda o binario; devolve o codigo de saida e o stdout capturado.
static Execucao rodar(const char *ambiente, const char *args) {
    Execucao e = {0};
    char cmd[2048];
    snprintf(cmd, sizeof cmd, "%s %s 2>/dev/null %s > build/tmp_stdout.txt",
             ambiente, BIN, args);
    // o redirecionamento de stderr vem antes do de stdout de proposito
    int r = system(cmd);
    e.status = (r == -1) ? -1 : (r >> 8) & 0xff;
    FILE *f = fopen("build/tmp_stdout.txt", "r");
    if (f) {
        size_t n = fread(e.saida, 1, sizeof e.saida - 1, f);
        e.saida[n] = '\0';
        fclose(f);
    }
    return e;
}

// Valor de uma chave do report ("CHAVE=valor\n"); -1 se nao existe.
static double valor(const Execucao *e, const char *chave) {
    char busca[64];
    snprintf(busca, sizeof busca, "%s=", chave);
    const char *p = e->saida;
    while ((p = strstr(p, busca)) != NULL) {
        if (p == e->saida || p[-1] == '\n') return atof(p + strlen(busca));
        p++;
    }
    return -1.0;
}

static bool tem_status(const Execucao *e, const char *s) {
    char busca[32];
    snprintf(busca, sizeof busca, "STATUS=%s\n", s);
    return strncmp(e->saida, busca, strlen(busca)) == 0;
}

static int cmp_str(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}

// Le as linhas de dados de um CSV (sem cabecalho). Devolve a quantidade.
static int ler_linhas(const char *caminho, char ***saida) {
    FILE *f = fopen(caminho, "r");
    if (!f) return -1;
    char **v = calloc(256, sizeof *v);
    char buf[4096];
    int n = 0;
    if (fgets(buf, sizeof buf, f) == NULL) { fclose(f); free(v); return -1; } // cabecalho
    while (fgets(buf, sizeof buf, f) && n < 256) {
        buf[strcspn(buf, "\r\n")] = '\0';
        v[n++] = strdup(buf);
    }
    fclose(f);
    *saida = v;
    return n;
}

static void liberar(char **v, int n) {
    for (int i = 0; i < n; i++) free(v[i]);
    free(v);
}

// Campo 'indice' (0-based) de uma linha CSV, respeitando aspas.
static double campo_numerico(const char *linha, int indice) {
    int atual = 0; bool dentro = false;
    const char *ini = linha;
    for (const char *p = linha;; p++) {
        if (*p == '"') dentro = !dentro;
        if ((*p == ',' && !dentro) || *p == '\0') {
            if (atual == indice) return atof(ini);
            atual++; ini = p + 1;
            if (*p == '\0') break;
        }
    }
    return -1e9;
}

static void args_nasa(char *dest, size_t n, const char *base, const char *res,
                      const char *consulta, const char *saida, int topk, int limit) {
    snprintf(dest, n,
        "--reader nasa --input %s/nasa_amostra.csv --base %s --result %s "
        "--query %s/%s --output %s --top-k %d --limit %d",
        DADOS, base, res, DADOS, consulta, saida, topk, limit);
}

// --------------------------------------------------------------- cenarios

static void report_basico_nasa(void) {
    char a[1024];
    args_nasa(a, sizeof a, "fila", "lista_ord", "consulta_nasa.txt",
              "build/tmp_pipe.csv", 0, 0);
    Execucao e = rodar("", a);
    CHECK_EQ_INT(e.status, 0);
    CHECK(tem_status(&e, "OK"));
    CHECK_EQ_INT(valor(&e, "INPUT_RECORDS"), 42);
    CHECK_EQ_INT(valor(&e, "MATCHED_RECORDS"), 24);
    CHECK_EQ_INT(valor(&e, "OUTPUT_RECORDS"), 24);
    CHECK(valor(&e, "LOAD_MS") >= 0);
    CHECK(valor(&e, "SEARCH_MS") >= 0);
    CHECK(valor(&e, "RESULT_INSERT_MS") >= 0);
    CHECK(valor(&e, "WRITE_MS") >= 0);
    // por padrao nenhuma metrica extra aparece
    CHECK(valor(&e, "PARSE_MS") < 0);

    char **linhas; int n = ler_linhas("build/tmp_pipe.csv", &linhas);
    CHECK_EQ_INT(n, 24);
    // lista_ord com ORDER risk_score DESC: coluna 11 nao crescente
    for (int i = 1; i < n; i++)
        CHECK(campo_numerico(linhas[i - 1], 11) >= campo_numerico(linhas[i], 11));
    for (int i = 0; i < n; i++) {
        CHECK(campo_numerico(linhas[i], 11) >= 3.0); // FILTER risk_score >= 3
        CHECK(campo_numerico(linhas[i], 3) >= 2020); // FILTER year >= 2020
    }
    liberar(linhas, n);
}

static void report_detalhado_por_ambiente(void) {
    char a[1024];
    args_nasa(a, sizeof a, "lista_enc", "fila", "consulta_nasa.txt",
              "build/tmp_pipe.csv", 0, 0);
    Execucao e = rodar("AED_REPORT_DETALHADO=1", a);
    CHECK_EQ_INT(e.status, 0);
    CHECK_EQ_INT(valor(&e, "INVALID_RECORDS"), 2);
    CHECK_EQ_INT(valor(&e, "BASE_INSERTED_RECORDS"), 40);
    CHECK_EQ_INT(valor(&e, "VISITED_RECORDS"), 40);
    CHECK_EQ_INT(valor(&e, "RESULT_INSERTED_RECORDS"), 24);
    double load = valor(&e, "LOAD_MS"), ins = valor(&e, "BASE_INSERT_MS"),
           parse = valor(&e, "PARSE_MS");
    CHECK(ins >= 0 && parse >= 0 && ins <= load + 0.002);
    CHECK_NEAR(ins + parse, load, 0.003); // PARSE = LOAD - BASE_INSERT (arredondamentos)

    Execucao zero = rodar("AED_REPORT_DETALHADO=0", a);
    CHECK(valor(&zero, "PARSE_MS") < 0);
}

static void top_k_e_limit(void) {
    char a[1024];
    args_nasa(a, sizeof a, "pilha", "lista_ord", "consulta_nasa.txt",
              "build/tmp_pipe.csv", 5, 0);
    Execucao e = rodar("", a);
    CHECK_EQ_INT(valor(&e, "MATCHED_RECORDS"), 24); // top-k nao limita a busca
    CHECK_EQ_INT(valor(&e, "OUTPUT_RECORDS"), 5); // so a exportacao
    char **l; int n = ler_linhas("build/tmp_pipe.csv", &l);
    CHECK_EQ_INT(n, 5);
    liberar(l, n);

    args_nasa(a, sizeof a, "fila", "fila", "consulta_nasa.txt",
              "build/tmp_pipe.csv", 0, 10);
    e = rodar("AED_REPORT_DETALHADO=1", a);
    CHECK_EQ_INT(valor(&e, "BASE_INSERTED_RECORDS"), 10); // --limit conta inseridos
    CHECK_EQ_INT(valor(&e, "VISITED_RECORDS"), 10);
}

// Todas as 64 combinacoes base x resultado devem selecionar o MESMO conjunto
// de registros; so a ordem de saida depende da estrutura de resultado.
static void todas_as_combinacoes_produzem_o_mesmo_conjunto(void) {
    char **ref = NULL; int nref = 0;
    for (int b = 0; b < N_EST; b++) {
        for (int r = 0; r < N_EST; r++) {
            char a[1024];
            args_nasa(a, sizeof a, ESTRUTURAS[b], ESTRUTURAS[r], "consulta_nasa.txt",
                      "build/tmp_comb.csv", 0, 0);
            Execucao e = rodar("", a);
            if (e.status != 0 || !tem_status(&e, "OK")) {
                fprintf(stderr, " falhou: base=%s result=%s\n",
                        ESTRUTURAS[b], ESTRUTURAS[r]);
                CHECK(false);
                continue;
            }
            CHECK_EQ_INT(valor(&e, "INPUT_RECORDS"), 42);
            CHECK_EQ_INT(valor(&e, "MATCHED_RECORDS"), 24);
            CHECK_EQ_INT(valor(&e, "OUTPUT_RECORDS"), 24);

            char **l; int n = ler_linhas("build/tmp_comb.csv", &l);
            CHECK_EQ_INT(n, 24);
            if (n < 0) continue;
            qsort(l, (size_t)n, sizeof *l, cmp_str);
            if (ref == NULL) { ref = l; nref = n; continue; }
            bool igual = (n == nref);
            for (int i = 0; igual && i < n; i++) igual = strcmp(l[i], ref[i]) == 0;
            if (!igual) fprintf(stderr, " conjunto diferente: base=%s result=%s\n",
                                ESTRUTURAS[b], ESTRUTURAS[r]);
            CHECK(igual);
            liberar(l, n);
        }
    }
    if (ref) liberar(ref, nref);
}

static void busca_por_similaridade(void) {
    char a[1024];
    args_nasa(a, sizeof a, "lista_seq", "lista_ord", "consulta_sim.txt",
              "build/tmp_pipe.csv", 3, 0);
    Execucao e = rodar("", a);
    CHECK_EQ_INT(e.status, 0);
    CHECK_EQ_INT(valor(&e, "MATCHED_RECORDS"), 24); // conferido a parte em Python
    CHECK_EQ_INT(valor(&e, "OUTPUT_RECORDS"), 3);
}

static void dataset_ancine(void) {
    char a[1024];
    snprintf(a, sizeof a,
        "--reader ancine --input %s/ancine_amostra.csv --base lista_cabeca "
        "--result lista_ord --query %s/consulta_ancine.txt --output build/tmp_anc.csv "
        "--top-k 0 --limit 0", DADOS, DADOS);
    Execucao e = rodar("AED_REPORT_DETALHADO=1", a);
    CHECK_EQ_INT(e.status, 0);
    CHECK_EQ_INT(valor(&e, "INPUT_RECORDS"), 10);
    CHECK_EQ_INT(valor(&e, "BASE_INSERTED_RECORDS"), 8);
    CHECK_EQ_INT(valor(&e, "INVALID_RECORDS"), 2);
    CHECK_EQ_INT(valor(&e, "MATCHED_RECORDS"), 5); // duracao > 90
    CHECK_EQ_INT(valor(&e, "OUTPUT_RECORDS"), 5);
    char **l; int n = ler_linhas("build/tmp_anc.csv", &l);
    CHECK_EQ_INT(n, 5);
    // ORDER duracao DESC: coluna 8 nao crescente (delimitador ';' -> usar strtod apos 8 ';')
    double anterior = 1e9;
    for (int i = 0; i < n; i++) {
        const char *p = l[i];
        for (int k = 0; k < 8; k++) p = strchr(p, ';') + 1;
        double d = atof(p);
        CHECK(d <= anterior); CHECK(d > 90);
        anterior = d;
    }
    liberar(l, n);
}

static void falhas_devolvem_status_error(void) {
    char a[1024];
    // arquivo de entrada inexistente: ainda imprime o report, com STATUS=ERROR e codigo 1
    snprintf(a, sizeof a,
        "--reader nasa --input build/nao_existe.csv --base fila --result fila "
        "--query %s/consulta_nasa.txt --output build/tmp_e.csv --top-k 0 --limit 0", DADOS);
    Execucao e = rodar("", a);
    CHECK_EQ_INT(e.status, 1);
    CHECK(tem_status(&e, "ERROR"));
    CHECK_EQ_INT(valor(&e, "OUTPUT_RECORDS"), 0);

    // reader desconhecido
    snprintf(a, sizeof a,
        "--reader xyz --input %s/nasa_amostra.csv --base fila --result fila "
        "--query %s/consulta_nasa.txt --output build/tmp_e.csv --top-k 0 --limit 0",
        DADOS, DADOS);
    e = rodar("", a);
    CHECK_EQ_INT(e.status, 1);
    CHECK(tem_status(&e, "ERROR"));

    // --output igual ao --input: recusado sem destruir a fonte
    char copia[256];
    snprintf(copia, sizeof copia, "cp %s/nasa_amostra.csv build/tmp_fonte.csv", DADOS);
    CHECK_EQ_INT(system(copia), 0);
    snprintf(a, sizeof a,
        "--reader nasa --input build/tmp_fonte.csv --base fila --result fila "
        "--query %s/consulta_nasa.txt --output build/tmp_fonte.csv --top-k 0 --limit 0",
        DADOS);
    e = rodar("", a);
    CHECK_EQ_INT(e.status, 1);
    char **l; int n = ler_linhas("build/tmp_fonte.csv", &l);
    CHECK_EQ_INT(n, 42);
    liberar(l, n);

    // consulta invalida e argumentos incompletos: nada de report, codigo 1
    snprintf(a, sizeof a,
        "--reader nasa --input %s/nasa_amostra.csv --base fila --result fila "
        "--query %s/nasa_amostra.csv --output build/tmp_e.csv --top-k 0 --limit 0",
        DADOS, DADOS);
    e = rodar("", a);
    CHECK_EQ_INT(e.status, 1);
    e = rodar("", "--reader nasa");
    CHECK_EQ_INT(e.status, 1);
    CHECK_EQ_INT(strlen(e.saida), 0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s <binario> <pasta_dados>\n", argv[0]);
        return 2;
    }
    BIN = argv[1];
    DADOS = argv[2];
    printf("test_pipeline (%s)\n", BIN);
    RUN(report_basico_nasa);
    RUN(report_detalhado_por_ambiente);
    RUN(top_k_e_limit);
    RUN(todas_as_combinacoes_produzem_o_mesmo_conjunto);
    RUN(busca_por_similaridade);
    RUN(dataset_ancine);
    RUN(falhas_devolvem_status_error);
    return testes_resumo("test_pipeline");
}