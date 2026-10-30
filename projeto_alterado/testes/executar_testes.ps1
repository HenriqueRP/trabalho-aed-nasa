param(
    [string]$DiretorioSaida = (Join-Path ([System.IO.Path]::GetTempPath()) ('aed-testes-' + [guid]::NewGuid()))
)

$ErrorActionPreference = 'Stop'
$taskProjeto = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path $DiretorioSaida | Out-Null
$taskSaida = (Resolve-Path -LiteralPath $DiretorioSaida).Path

# Cada executavel liga apenas os modulos necessarios, sem depender do main incompleto
$taskIncludes = Get-ChildItem -LiteralPath $taskProjeto -Recurse -Filter '*.h' |
    ForEach-Object { $_.DirectoryName } | Sort-Object -Unique |
    ForEach-Object { '-I' + $_ }
$taskComuns = @(
    (Join-Path $taskProjeto 'eds/tad_item/item.c'),
    (Join-Path $taskProjeto 'query/query.c')
)
$taskCasos = @(
    @{Nome = 'lista_cabeca'; Fontes = @('eds/lista_cabeca/lista_cabeca.c')}
)

foreach ($taskCaso in $taskCasos) {
    $taskExecutavel = Join-Path $taskSaida ('test_' + $taskCaso.Nome + '.exe')
    $taskFontes = @((Join-Path $PSScriptRoot ('test_' + $taskCaso.Nome + '.c')))
    $taskFontes += $taskComuns
    $taskFontes += $taskCaso.Fontes | ForEach-Object { Join-Path $taskProjeto $_ }

    & gcc -std=c11 -Wall -Wextra -pedantic -Werror -g @taskIncludes @taskFontes -lm -o $taskExecutavel
    if ($LASTEXITCODE -ne 0) { throw ('Falha ao compilar ' + $taskCaso.Nome) }
    & $taskExecutavel
    if ($LASTEXITCODE -ne 0) { throw ('Falha ao executar ' + $taskCaso.Nome) }
}
