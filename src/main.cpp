#include <iostream>
#include <fstream>
#include <string>
#include "hash_index.h"

using namespace std;

// ---------------------------------------------------------------------------
// Ponto de entrada do programa
//
// Lê o arquivo "in.txt" linha a linha e executa cada operação do índice hash.
// Os resultados são escritos em "out.txt" e também impressos no terminal.
//
// Formato de in.txt:
//   PG/<profundidade global inicial>
//   INC:x
//   REM:x
//   BUS=:x
//   ...
//
// Formato de out.txt:
//   PG/<profundidade global inicial>   ← cópia da 1ª linha de in.txt
//   [DUP DIR:/<pg>,<pl>]               ← quando INC duplica o diretório
//   INC:x/<pg>,<pl>
//   REM:x/<qtd>,<pg>,<pl>
//   BUS:x/<qtd>
//   ...
//   P:/<profundidade global final>     ← última linha
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {

    // ------------------------------------------------------------------
    // Nomes dos arquivos de entrada e saída.
    // Por padrão usa in.txt / out.txt, mas aceita nomes via argumento:
    //   ./sgbd_hash comandos.csv
    // ------------------------------------------------------------------
    string nomeEntrada = (argc >= 2) ? argv[1] : DATA_DIR + "in.txt";
    string nomeSaida   = "out.txt";

    ifstream arqEntrada(nomeEntrada);
    if (!arqEntrada.is_open()) {
        cerr << "[ERRO] Não foi possível abrir '" << nomeEntrada << "'." << endl;
        cerr << "       Uso: ./sgbd_hash [arquivo_entrada]  (padrão: in.txt)" << endl;
        return 1;
    }

    ofstream arqSaida(nomeSaida);
    if (!arqSaida.is_open()) {
        cerr << "[ERRO] Não foi possível abrir/criar '" << nomeSaida << "'." << endl;
        return 1;
    }

    // ------------------------------------------------------------------
    // Leitura da primeira linha: PG/<profundidade global inicial>
    // ------------------------------------------------------------------
    string linha;
    getline(arqEntrada, linha);

    if (linha.size() < 4 || linha.substr(0, 3) != "PG/") {
        cerr << "[ERRO] Primeira linha inválida (esperado 'PG/<n>'): " << linha << endl;
        return 1;
    }

    unsigned int pg_inicial = stoul(linha.substr(3));

    // Reproduz a primeira linha no arquivo de saída
    arqSaida << linha << "\n";
    cout << linha << endl;

    // ------------------------------------------------------------------
    // Inicialização do diretório hash
    // (carrega do disco se existir; cria do zero caso contrário)
    // ------------------------------------------------------------------
    Diretorio dir = inicializarDiretorio(pg_inicial);

    // ------------------------------------------------------------------
    // Loop principal: processa cada operação do arquivo de entrada
    // ------------------------------------------------------------------
    while (getline(arqEntrada, linha)) {

        // Ignora linhas em branco
        if (linha.empty()) continue;

        if (linha.substr(0, 4) == "INC:") {
            // --------------------------------------------------------
            // Inclusão de chave no índice — implementada pelo colega
            // --------------------------------------------------------
            unsigned int x = stoul(linha.substr(4));
            operacaoINC(x, dir, arqSaida);

        } else if (linha.substr(0, 4) == "REM:") {
            // --------------------------------------------------------
            // Remoção de chave do índice — implementada pelo colega
            // --------------------------------------------------------
            unsigned int x = stoul(linha.substr(4));
            operacaoREM(x, dir, arqSaida);

        } else if (linha.substr(0, 5) == "BUS=:") {
            // --------------------------------------------------------
            // Busca por igualdade — implementada em hash_index.cpp
            // --------------------------------------------------------
            unsigned int x = stoul(linha.substr(5));
            operacaoBUS(x, dir, arqSaida);

        } else {
            cerr << "[AVISO] Operação desconhecida ignorada: " << linha << endl;
        }
    }

    // ------------------------------------------------------------------
    // Última linha obrigatória do arquivo de saída
    // ------------------------------------------------------------------
    arqSaida << "P:/" << dir.pg << "\n";
    cout << "P:/" << dir.pg << endl;

    arqEntrada.close();
    arqSaida.close();

    return 0;
}