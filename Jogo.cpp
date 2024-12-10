#include <iostream>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <semaphore.h>
#include <atomic>
using namespace std;

//************************************************************************************************************************/

// ADHEMAR MOLON NETO - 14687681
// Luiz Fellipe Catuzzi - 11871198
// GUILHERME ANTONIO COSTA BANDEIRA - 14575620
// SEU NOME - SEU NÚMERO USP

// O código cria um jogo simples de labirinto onde o nosso pequeno arroba aventureiro deve coletar todos os (*) sem tocar nas paredes (#).

// A função `getch` faz com que o arroba aventureiro se movimente melhor (captura uma tecla pressionada sem precisar apertar ENTER)
// A função `mostrarHistorinha` mostra a historia com um pequeno tutorial
// A função `printLabirinto` limpa o terminal e mostra o labirinto atualizado, contando tambem os * coletados e a quantidade de passos
// A função `jogarNivel` é o caule do jogo. Ela:
// 1. Spawna o nosso pequeno arroba aventureiro no labirinto e o mantém em loop até que o nível seja completado (ou ele morra :v)
// 2. Pega os W,A,S,D do arroba aventureiro e chama a função `processarComando` para lidar com o movimento
// 3. Verifica se o arroba aventureiro morreu ou completou o nível
// 4. Atualiza o labirinto quando o arroba aventureiro dá um passo e termina o nível quando todos os * são coletados.
// A função `contarItens` apenas conta os itens (*) no labirinto para saber quando o nível está completo.
// A função `processarComando` lida com o movimento do jogador, verificando se ele perdeu, pegou um item ou completou o nível.
// A função `contadorRegressivo` controla o tempo limite para completar cada nível, encerrando o jogo se o tempo acabar.
// Na função `main`, o jogo começa com a introdução, passa pelos níveis e ao final mostra quantos passos o jogador deu no total.
// Resumindo: o jogador usa W, A, S, D para se mover, coleta todos os itens e tenta não encostar nas paredes. Completar os níveis é a condição de vitória.

//************************************************************************************************************************/

// Funções e variáveis globais
sem_t labirinto_sem; // Semáforo para sincronizar acesso ao labirinto
atomic<bool> tempo_acabou(false); // Indica se o tempo do nível acabou
atomic<bool> nivel_completado(false); // Indica se o nível foi completado

void mostrarHistorinha();
void printLabirinto(const vector<vector<char>> &labirinto, int contador, int totalItens, int passos);
void jogarNivel(vector<vector<char>> labirinto, int &totalPassos);
int contarItens(const vector<vector<char>> &labirinto);
bool processarComando(vector<vector<char>> &labirinto, char comando, int &jogadorX, int &jogadorY, int &contador, int &passos, int &totalPassos, int totalItens);
void contadorRegressivo(int segundos);

// Função para capturar entrada sem pressionar ENTER
char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0) perror("tcgetattr");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr");
    if (read(0, &buf, 1) < 0) perror("read");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr");
    return buf;
}

//************************************************************************************************************************/

// Função que mostra a historinha do jogo
void mostrarHistorinha() {
    cout << "Bem-vindo ao desafio do labirinto!\n\n";
    cout << "Você é um pequeno arroba aventureiro (@) que precisa coletar todos os tesouros (*) e escapar sem tocar nas paredes (#).\n";
    cout << "Use as teclas W, A, S e D para se movimentar.\n";
    cout << "Pressione ENTER para começar sua jornada...";
    cin.ignore();
}

// Função que imprime o labirinto
void printLabirinto(const vector<vector<char>> &labirinto, int contador, int totalItens, int passos) {
    sem_wait(&labirinto_sem); // Espera pelo semáforo
    system("clear");
    for (const auto &linha : labirinto) {
        for (char c : linha) {
            cout << c << ' ';
        }
        cout << endl;
    }
    cout << "\nItens coletados: " << contador << " de " << totalItens;
    cout << " | Passos: " << passos << endl;
    sem_post(&labirinto_sem); // Libera o semáforo
}

// Função que controla a jogabilidade de cada nível
void jogarNivel(vector<vector<char>> labirinto, int &totalPassos) {
    int jogadorX = 1, jogadorY = 1; // Posição inicial do jogador
    int contador = 0; // Quantidade de itens (*) coletados
    int totalItens = contarItens(labirinto); // Conta os itens no labirinto
    int passos = 0; // Número de passos dados no nível

    tempo_acabou = false;
    nivel_completado = false;
    thread timer(contadorRegressivo, 30); // Thread para o contador regressivo (30 segundos por nível)

    printLabirinto(labirinto, contador, totalItens, passos);

    while (!tempo_acabou && !nivel_completado) {
        char comando = getch(); // Lê comando do jogador
        if (processarComando(labirinto, comando, jogadorX, jogadorY, contador, passos, totalPassos, totalItens)) {
            nivel_completado = true; // Marca o nível como concluído
        }
        printLabirinto(labirinto, contador, totalItens, passos);
    }

    timer.join(); // Aguarda a thread terminar

    if (tempo_acabou) {
        system("clear");
        cout << "\nO tempo acabou! Você perdeu o jogo.\n";
        exit(0);
    }
}

// Conta os itens (*) no labirinto
int contarItens(const vector<vector<char>> &labirinto) {
    int total = 0;
    for (const auto &linha : labirinto) {
        for (char c : linha) {
            if (c == '*') total++;
        }
    }
    return total;
}

// Processa o comando do jogador e atualiza o labirinto
bool processarComando(vector<vector<char>> &labirinto, char comando, int &jogadorX, int &jogadorY, int &contador, int &passos, int &totalPassos, int totalItens) {
    int novoX = jogadorX, novoY = jogadorY;

    // Movimenta o jogador conforme o comando
    if (comando == 'w') novoX--;
    else if (comando == 's') novoX++;
    else if (comando == 'a') novoY--;
    else if (comando == 'd') novoY++;
    else if (comando == 'q') exit(0); // Sai do jogo

    sem_wait(&labirinto_sem); // Espera pelo semáforo

    // Verifica colisões ou coleta de itens
    if (labirinto[novoX][novoY] == '#') {
        system("clear");
        cout << "\nVocê tentou atravessar uma parede e perdeu!\nFim de jogo.\n";
        exit(0);
    } else if (labirinto[novoX][novoY] == '*') {
        contador++; // Incrementa contador ao pegar item
    }

    // Movimenta o arroba
    labirinto[jogadorX][jogadorY] = ' '; // Apaga posição anterior
    jogadorX = novoX;
    jogadorY = novoY;
    labirinto[jogadorX][jogadorY] = '@'; // Atualiza nova posição

    passos++;
    totalPassos++;

    sem_post(&labirinto_sem); // Libera o semáforo

    // Verifica condição de vitória
    if (contador == totalItens) {
        system("clear");
        cout << "\nParabéns! Você completou o nível com " << passos << " passos.\n";
        cout << "Pressione ENTER para continuar para o próximo nível...";
        cin.ignore();
        return true;
    }
    return false;
}

// Thread que controla o tempo do nível
void contadorRegressivo(int segundos) {
    while (segundos > 0 && !nivel_completado) {
        sem_wait(&labirinto_sem); // Espera pelo semáforo
        cout << "\rTempo restante: " << segundos-- << " segundos." << flush;
        sem_post(&labirinto_sem); // Libera o semáforo
        this_thread::sleep_for(chrono::seconds(1));
    }
    if (!nivel_completado) {
        tempo_acabou = true; // Sinaliza que o tempo acabou
    }
}

// Função principal
int main() {
    sem_init(&labirinto_sem, 0, 1); // Inicializa o semáforo

    mostrarHistorinha();

    vector<vector<char>> nivel1 = {
        {'#', '#', '#', '#', '#'},
        {'#', '@', '*', ' ', '#'},
        {'#', ' ', '#', ' ', '#'},
        {'#', '*', ' ', '*', '#'},
        {'#', '#', '#', '#', '#'}};

    vector<vector<char>> nivel2 = {
        {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#'},
        {'#', '@', ' ', '*', '#', ' ', ' ', '*', ' ', '#'},
        {'#', ' ', '#', ' ', '#', ' ', '#', ' ', '#', '#'},
        {'#', ' ', '#', '*', ' ', ' ', '#', ' ', ' ', '#'},
        {'#', ' ', '#', '#', '#', ' ', '#', '*', ' ', '#'},
        {'#', ' ', ' ', ' ', '#', '*', '#', ' ', ' ', '#'},
        {'#', ' ', '#', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
        {'#', '*', '#', '#', '#', '#', '#', '#', '*', '#'},
        {'#', ' ', '*', ' ', ' ', '*', '#', ' ', ' ', '#'},
        {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#'}};

    int totalPassos = 0;

    cout << "\nIniciando Nível 1...\n";
    jogarNivel(nivel1, totalPassos);

    cout << "\nIniciando Nível 2...\n";
    jogarNivel(nivel2, totalPassos);

    cout << "\nParabéns! Você completou o jogo com um total de " << totalPassos << " passos.\n";

    sem_destroy(&labirinto_sem); // Destroi o semáforo
    return 0;
}




// COMO AS THREADS FUNCIONAM NO CODIGO ?
// As threads permitem que o temporizador funcione em paralelo com a jogabilidade. 
// A função `contadorRegressivo` é executada em uma thread separada, controlando o tempo restante e finalizando o jogo se o tempo acabar.

// COMO O SEMAFORO FUNCIONA NO CODIGO ?
// O semáforo `labirinto_sem` sincroniza o acesso ao terminal e ao labirinto. 
// Ele garante que apenas uma thread (jogador ou temporizador) modifique ou exiba informações no terminal de cada vez, evitando conflitos.
