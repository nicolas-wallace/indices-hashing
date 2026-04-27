#ifndef HASH_INDEX_H
#define HASH_INDEX_H

#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

using namespace std;

// ---------------------------------------------------------------------------
// Constantes
// ---------------------------------------------------------------------------

// Número máximo de entradas de dados por bucket
const int BUCKET_CAP = 5;

// Pastas usadas pelo programa
const string INDEX_DIR = "index/";   // buckets e diretório do índice
const string DATA_DIR  = "data/";    // dataset e arquivo de entrada

// Caminhos completos dos arquivos persistentes
const string ARQUIVO_TEXTO     = DATA_DIR  + "bd-trab2-dataset.csv";
const string ARQUIVO_DIRETORIO = INDEX_DIR + "diretorio.txt";

// ---------------------------------------------------------------------------
// Stream global que representa a ÚNICA página em memória secundária por vez.
// Toda leitura/escrita de bucket deve usar este fstream e fechá-lo antes de
// abrir qualquer outro arquivo de bucket.
// Exceção permitida: o diretório pode permanecer em memória (struct Diretorio).
// ---------------------------------------------------------------------------
extern fstream pagina;

// ---------------------------------------------------------------------------
// Estrutura do Diretório do Índice Hash Extensível
// ---------------------------------------------------------------------------
typedef struct {
    unsigned int pg;                  // Profundidade global
    vector<unsigned int> bucket_ref;  // bucket_ref[i] = id numérico do bucket
                                      // Ex.: bucket_ref[3] = 5  →  arquivo "5.txt"
} Diretorio;

// ---------------------------------------------------------------------------
// Protótipos — implementados em hash_index.cpp (suas partes)
// ---------------------------------------------------------------------------

/**
 * Retorna os 'pg' bits menos significativos de x.
 * Essa é a função hash usada para mapear uma chave ao índice do diretório.
 * Exemplo: funcaoHash(13, 4) → 13 & 0b1111 = 13
 */
unsigned int funcaoHash(unsigned int x, unsigned int pg);

/**
 * Carrega o diretório do disco (ARQUIVO_DIRETORIO).
 * Se o arquivo não existir, cria um diretório novo com profundidade pg_inicial,
 * gerando os 2^pg_inicial arquivos de bucket vazios.
 */
Diretorio inicializarDiretorio(unsigned int pg_inicial);

/**
 * Persiste o diretório atual no disco (sobrescreve ARQUIVO_DIRETORIO).
 * Deve ser chamada sempre que o diretório for modificado.
 */
void salvarDiretorio(const Diretorio& dir);

/**
 * Operação BUS=:x — busca por igualdade no índice hash.
 * Escreve no arqSaida a linha: BUS:x/<quantidade de tuplas selecionadas>
 */
void operacaoBUS(unsigned int x, const Diretorio& dir, ofstream& arqSaida);

// ---------------------------------------------------------------------------
// Protótipos — implementados em operacoes.cpp
// ---------------------------------------------------------------------------

/**
 * Operação INC:x — insere a chave x no índice hash.
 * Deve escrever em arqSaida:
 *   [DUP DIR:/<pg>,<pl>]   ← se o diretório for duplicado (antes da linha INC)
 *   INC:x/<pg>,<pl>
 */
void operacaoINC(unsigned int x, Diretorio& dir, ofstream& arqSaida);

/**
 * Operação REM:x — remove a chave x do índice hash.
 * Deve escrever em arqSaida:
 *   REM:x/<qtd removidas>,<pg>,<pl>
 */
void operacaoREM(unsigned int x, Diretorio& dir, ofstream& arqSaida);

#endif // HASH_INDEX_H