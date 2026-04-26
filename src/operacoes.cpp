#include "hash_index.h"

// ===========================================================================
// operacoes.cpp — Implementação das operações INC e REM
// Responsável: colega
//
// Lembre-se das regras:
//   • Use sempre 'pagina' (fstream global) para ler/escrever buckets.
//   • Feche 'pagina' antes de abrir outro bucket.
//   • O diretório (struct Diretorio) pode ficar em memória normalmente.
//   • Chame salvarDiretorio(dir) sempre que modificar dir.
//   • Buckets são armazenados como "<id>.txt":
//       Linha 1 = profundidade local (PL)
//       Linhas 2..N = entradas (LinhaNum), até BUCKET_CAP entradas por bucket.
// ===========================================================================

// ---------------------------------------------------------------------------
// operacaoINC — Insere a chave x no índice hash extensível
//
// Algoritmo geral (hash extensível):
//   1. Calcular idx = funcaoHash(x, dir.pg)
//   2. Abrir o bucket dir.bucket_ref[idx]
//   3. Se bucket não está cheio: inserir x e fechar
//   4. Se bucket está cheio:
//      a. Se PL < PG: dividir o bucket (split local)
//      b. Se PL == PG: duplicar o diretório e depois fazer split
//
// Formato de saída (em arqSaida e cout):
//   [DUP DIR:/<pg>,<pl>\n]   ← escrever ANTES da linha INC, se houve duplicação
//   INC:x/<pg>,<pl>\n
// ---------------------------------------------------------------------------
void operacaoINC(unsigned int x, Diretorio& dir, ofstream& arqSaida) {
    // TODO: implementar
    cout << "[TODO] INC:" << x << " — a implementar" << endl;
}

// ---------------------------------------------------------------------------
// operacaoREM — Remove a chave x do índice hash extensível
//
// Algoritmo geral:
//   1. Calcular idx = funcaoHash(x, dir.pg)
//   2. Abrir o bucket dir.bucket_ref[idx]
//   3. Remover a entrada x (se existir); contar quantas foram removidas (0 ou 1)
//   4. (Opcional) Verificar possibilidade de merge de buckets
//
// Formato de saída (em arqSaida e cout):
//   REM:x/<qtd removidas>,<pg>,<pl>\n
// ---------------------------------------------------------------------------
void operacaoREM(unsigned int x, Diretorio& dir, ofstream& arqSaida) {
    // TODO: implementar
    cout << "[TODO] REM:" << x << " — a implementar" << endl;
}