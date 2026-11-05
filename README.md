# Trabalho de AED — pipeline NASA / ANCINE

Programa em C que lê um CSV, guarda os registros em uma das **8 estruturas de
dados** (`--base`), aplica uma consulta (`FILTER`, `SIM`, `ACCEPT`), guarda os
aceitos em outra estrutura (`--result`), exporta o resultado em CSV e imprime
um relatório com contagens e tempos.

```
CSV --reader--> BASE --search(Query)--> RESULT --writer--> CSV de saida
                  \________________ report (stdout) ________________/
```

## Compilar, testar e executar

Requer GNU make e gcc/clang (C11). No Windows, use MSYS2 ou WSL.

| Comando | O que faz |
|---|---|
| `make` | compila `bin/aed` (objetos em `build/`) |
| `make test` | testes unitarios + teste de pipeline completo |
| `make unit` / `make integration` | so uma das duas partes |
| `make valgrind` | testes unitarios sob valgrind (erros de memoria e vazamentos) |
| `make run` / `make run-detalhado` | roda o exemplo sobre `data/` |
| `make clean` | apaga `build/` e `bin/` |

## Uso

```
bin/aed --reader nasa|ancine --input ARQ.csv \
        --base ESTRUTURA --result ESTRUTURA \
        --query CONSULTA.txt --output SAIDA.csv \
        --top-k N --limit N
```

Os 8 pares sao obrigatorios, em qualquer ordem. `ESTRUTURA` e uma entre
`pilha`, `fila`, `lista_seq`, `lista_enc`, `lista_cabeca`, `lista_ord`,
`lista_gen`, `lista_cruz`. `--top-k` limita so a **exportacao** (0 = tudo) e
`--limit` limita quantos registros validos sao **inseridos na base** (0 = todos).
`lista_ord`, `lista_gen` e `lista_cruz` exigem `ORDER`, `GROUP` e `CROSS`
na consulta, respectivamente.

### Consulta (`data/consulta_*.txt`)

```
FILTER|campo|op|valor      # op: == != < <= > >=  (varios FILTER = AND)
SIM|campo|alvo|tolerancia|peso
ACCEPT|limiar              # 0..1, nota minima da similaridade
ORDER|campo|ASC|DESC       # campo "similarity" ordena pela nota
GROUP|campo1|campo2
CROSS|campo1|campo2
```

## Report

Saida padrao (`stdout`; diagnosticos vao para `stderr`):

```
STATUS=OK
INPUT_RECORDS=42        linhas de dados lidas
MATCHED_RECORDS=24      aceitos pela consulta
OUTPUT_RECORDS=5        escritos no CSV (respeita --top-k)
LOAD_MS=0.093           leitura total (parse + insercao na BASE)
SEARCH_MS=0.015         busca, sem o tempo de insercao no RESULT
RESULT_INSERT_MS=0.005  somente a insercao no RESULT
WRITE_MS=0.078          exportacao
```

Com `AED_REPORT_DETALHADO=1` o report acrescenta, apos `WRITE_MS`:
`INVALID_RECORDS`, `BASE_INSERTED_RECORDS`, `VISITED_RECORDS`,
`RESULT_INSERTED_RECORDS`, `BASE_INSERT_MS` (tempo dentro de
`estrutura_inserir` da BASE) e `PARSE_MS` (= `LOAD_MS` − `BASE_INSERT_MS`).
Assim da para comparar o custo de insercao das estruturas separado do custo
de ler o CSV. O formato padrao nao muda.

## Estrutura do repositorio

```
src/
  main/      main.c, config (argv), report
  reader/    fachada opaca + readers nasa e ancine
  query/     parser e avaliacao de FILTER/SIM/ACCEPT/ORDER/GROUP/CROSS
  search/    percorre a BASE e insere copias no RESULT (com metricas)
  writer/    exporta o RESULT para CSV
  util/      timer (relogio monotono compartilhado)
  eds/       as 8 estruturas, a fachada `estrutura` e os TADs de item
tests/
  test_util.[ch]   mini framework (CHECK, RUN) e helpers
  unit/            um executavel por modulo
  integration/     test_pipeline: roda bin/aed de ponta a ponta
data/              amostras (NASA e ANCINE) e consultas de exemplo
.github/workflows/ci.yml   make, make test, make valgrind
```

Convencoes: cada modulo expoe um header com tipos opacos; `ITEM` guarda um
dado generico (`void*` + apagar/copiar/ler_campo), entao estruturas, busca e
escrita nao conhecem o dataset. Uma estrutura so assume a posse de um `ITEM`
quando a insercao devolve `true`.

## Testes

* **Unitarios** (`tests/unit`): as 8 estruturas isoladas e pela fachada
  (ordem, posse, percurso interrompido, ponteiros nulos), item/query
  (filtros, similaridade, media ponderada, consultas invalidas), config,
  reader (linhas invalidas, aspas, CRLF, `--limit`), search (metricas,
  copias independentes, 64 combinacoes BASE × RESULT), writer (top-k,
  cabecalho, protecao do arquivo de entrada) e report (formato exato e
  modo detalhado).
* **Pipeline** (`tests/integration/test_pipeline.c`): executa `bin/aed` para
  as 64 combinacoes `--base` × `--result` e confere que todas selecionam o
  mesmo conjunto de registros; confere o report, `--top-k`, `--limit`,
  ANCINE e os caminhos de erro (`STATUS=ERROR`, saida igual a entrada etc.).
