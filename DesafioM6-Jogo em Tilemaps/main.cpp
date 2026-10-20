/*
 * Desafio M6 - Jogo em Tilemaps: DUNGEON ESCAPE
 *
 * O cavaleiro está preso numa masmorra (visão de cima). Para escapar ele precisa
 * abrir o baú que guarda a chave e depois sair pela porta, antes que o tempo acabe
 * e sem perder todas as vidas.
 *
 * Cenário:
 *   - Tilemap lido de "mapa.txt" (um caractere por tile) e desenhado com o tileset
 *     Kenney Tiny Dungeon (16x16, escala 3x).
 *   - Autotile: o topo das paredes é montado em 4 quadrantes (cada um escolhido
 *     pelos vizinhos), as paredes com chão embaixo viram faces de tijolos e o chão
 *     recebe sombra quando há parede acima/à esquerda.
 *   - Iluminação simples: masmorra escura, com luz em volta do jogador e dos itens.
 *
 * Elementos que mudam de posição: rato patrulhando, morcegos voando em circuito,
 * espinhos que sobem e descem e a porta que se abre.
 *
 * Ganha: abre o baú (chave) e entra na porta aberta.
 * Perde: perde as 3 vidas (monstros, espinhos) ou o tempo acaba.
 *
 * Controles:
 *   WASD / setas - move   ENTER - começa   P - pausa   R - reinicia
 *   G - mostra a grade do tilemap   ESC - sai
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ---------------- Configurações ----------------
const int TILE = 16;                 // tamanho do tile no tileset (px)
const int ESCALA = 3;                // cada pixel do tileset vira 3x3 na tela
const float T = TILE * ESCALA;       // tamanho do tile na tela (48 px)
const int COLUNAS_ATLAS = 12, LINHAS_ATLAS = 11;
const float VEL_JOGADOR = 6.0f;      // tiles por segundo
const int VIDAS = 3;
const float TEMPO_LIMITE = 180.0f;   // segundos
const float INVULNERAVEL = 1.5f;     // tempo piscando depois de levar dano
const float PERIODO_ESPINHOS = 2.4f, ESPINHOS_ALTO = 1.0f, AVISO_ESPINHOS = 0.4f;
const float AMBIENTE = 0.28f;
const int MAX_LUZES = 16;

// Flags de desenho (aplicadas na coordenada de textura no vertex shader)
const int ESPELHAR = 1, TRANSPOR = 2;

// Índices dos tiles em tilemap_packed.png (linha * 12 + coluna)
namespace Tile {
const int TOPO = 0;
// Borda do topo da parede (o lado com "buraco" é o lado onde há chão ou face)
const int BORDA_N = 26, BORDA_S = 2, BORDA_L = 13, BORDA_O = 15;               // L = leste, O = oeste
const int QUINA_NO = 4, QUINA_NE = 5, QUINA_SO = 16, QUINA_SE = 17;           // cantos convexos
const int CANTO_NO = 27, CANTO_NE = 25, CANTO_SO = 3, CANTO_SE = 1;           // cantos côncavos
const int FACE = 40, FACE_ESQ = 57, FACE_AMBOS = 58, FACE_DIR = 59, GRADE = 28, TOCHA = 29;
const int PORTA[4] = {45, 33, 21, 9};                                          // fechada -> aberta
const int CHAO = 48, CHAO2 = 49, SOMBRA = 50, SOMBRA2 = 51, SOMBRA_CANTO = 52, SOMBRA_PONTA = 53;
const int ESPINHOS = 41;
const int CRUZ = 64, LAPIDE = 65, BARRIL = 66, MESA = 72, BANCO = 73, BIGORNA = 74;
const int BAU = 89, BAU_ABERTO = 91, POCAO = 115;
const int CAVALEIRO = 96, SPR_MORCEGO = 120, SPR_RATO = 123;
} // namespace Tile

// ---------------- Fonte bitmap 5x7 (não há fonte no tileset) ----------------
const std::string CARACTERES = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:!.-/+~ ";
const char *FONTE[][7] = {
    {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // A
    {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}, // B
    {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}, // C
    {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}, // D
    {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}, // E
    {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}, // F
    {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}, // G
    {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // H
    {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, // I
    {"..###", "...#.", "...#.", "...#.", "#..#.", "#..#.", ".##.."}, // J
    {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}, // K
    {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}, // L
    {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}, // M
    {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}, // N
    {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // O
    {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}, // P
    {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}, // Q
    {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}, // R
    {".####", "#....", "#....", ".###.", "....#", "....#", "####."}, // S
    {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}, // T
    {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // U
    {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}, // V
    {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}, // W
    {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}, // X
    {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}, // Y
    {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}, // Z
    {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}, // 0
    {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}, // 1
    {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}, // 2
    {"####.", "....#", "....#", ".###.", "....#", "....#", "####."}, // 3
    {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}, // 4
    {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}, // 5
    {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."}, // 6
    {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}, // 7
    {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}, // 8
    {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."}, // 9
    {".....", "..#..", "..#..", ".....", "..#..", "..#..", "....."}, // :
    {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}, // !
    {".....", ".....", ".....", ".....", ".....", ".....", "..#.."}, // .
    {".....", ".....", ".....", "#####", ".....", ".....", "....."}, // -
    {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}, // /
    {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}, // +
    {".###.", "#...#", ".###.", "..#..", "..##.", "..#..", "..##."}, // ~ (ícone da chave)
    {".....", ".....", ".....", ".....", ".....", ".....", "....."}, // espaço
};

// ---------------- Tipos ----------------
enum Estado { INICIO, JOGANDO, VITORIA, DERROTA };
enum TipoInimigo { RATO, MORCEGO };
enum Modo { VAIVEM, CIRCUITO };

struct Mapa {
    int largura = 0, altura = 0;
    std::vector<std::string> linhas;
    char em(int x, int y) const {
        if (x < 0 || y < 0 || x >= largura || y >= altura) return '#';
        return linhas[y][x];
    }
    bool parede(int x, int y) const {
        char c = em(x, y);
        return c == '#' || c == 'T' || c == 'G' || c == 'E';
    }
    // Face: parede com chão logo abaixo (mostra os tijolos). Topo: o resto da parede.
    bool face(int x, int y) const { return parede(x, y) && y + 1 < altura && !parede(x, y + 1); }
    bool topo(int x, int y) const { return parede(x, y) && !face(x, y); }
};

// Pedaço de tile pré-calculado da camada estática (chão, paredes, decoração)
struct Peca {
    int tile;
    glm::vec2 centro, tamanho;
    glm::vec4 sub; // sub-retângulo do tile (x, y, largura, altura em frações do tile)
    int flags;
};

struct Inimigo {
    TipoInimigo tipo;
    Modo modo;
    float vel;
    std::vector<glm::vec2> pontos; // em coordenadas de tile (centro do tile = x + 0.5)
    glm::vec2 pos;
    int alvo = 0, sentido = 1;
    bool esquerda = false;
    float fase = 0.0f;
};

struct Pocao {
    glm::ivec2 tile;
    bool pega;
};

struct Jogador {
    glm::ivec2 tile, inicio;
    glm::vec2 pos, origem;
    bool movendo = false, esquerda = false;
    float progresso = 0.0f, invulneravel = 0.0f;
};

// ---------------- Estado do jogo ----------------
Mapa mapa;
std::vector<Peca> camadaBase;
std::vector<glm::ivec2> espinhos;
std::vector<Inimigo> inimigosIniciais, inimigos;
std::vector<Pocao> pocoes;
glm::ivec2 posBau(-1), posPorta(-1);
Jogador jogador;
Estado estado = INICIO;
float tempo = 0.0f, tempoEstado = 0.0f, tempoRestante = TEMPO_LIMITE;
int vidas = VIDAS;
bool temChave = false, pausado = false, mostrarGrade = false;
float aberturaPorta = 0.0f, tempoChave = 0.0f, flashDano = 0.0f;
glm::ivec2 checkpoint;
std::string mensagem, motivoDerrota;
float tempoMensagem = 0.0f;
int WIDTH = 0, HEIGHT = 0;

GLuint texAtlas, texFonte;
int larguraFonte = 1;
GLuint VAO;
GLint modelLoc, uvRectLoc, flagsLoc, corLoc, solidoLoc, iluminarLoc, ambienteLoc, numLuzesLoc, luzesLoc, coresLuzLoc;

// ---------------- Shaders ----------------
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 vp;
uniform mat4 projection;
uniform mat4 model;
uniform vec4 uvRect;   // canto (xy) e tamanho (zw) do pedaço da textura
uniform int flags;     // 1 = espelha na horizontal, 2 = transpõe (troca x e y)
out vec2 uv;
out vec2 posMundo;
void main() {
    vec2 l = vp + 0.5;
    if ((flags & 2) != 0) l = l.yx;
    if ((flags & 1) != 0) l.x = 1.0 - l.x;
    uv = uvRect.xy + l * uvRect.zw;
    vec4 w = model * vec4(vp, 0.0, 1.0);
    posMundo = w.xy;
    gl_Position = projection * w;
}
)";

// Iluminação: cada luz (x, y, raio, intensidade) soma uma contribuição que cai com a
// distância. A posição é arredondada para o "pixel" do tileset para manter o visual pixel art.
const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec2 uv;
in vec2 posMundo;
uniform sampler2D tex;
uniform vec4 cor;
uniform bool solido;
uniform bool iluminar;
uniform float ambiente;
uniform int numLuzes;
uniform vec4 luzes[16];
uniform vec3 coresLuz[16];
out vec4 frg;
void main() {
    vec4 c = solido ? cor : texture(tex, uv) * cor;
    if (c.a < 0.01) discard;
    if (iluminar) {
        vec2 p = (floor(posMundo / 3.0) + 0.5) * 3.0;
        vec3 luz = vec3(ambiente);
        for (int i = 0; i < numLuzes; i++) {
            float f = clamp(1.0 - distance(p, luzes[i].xy) / luzes[i].z, 0.0, 1.0);
            luz += coresLuz[i] * luzes[i].w * f * f;
        }
        c.rgb *= min(luz, vec3(1.25));
    }
    frg = c;
}
)";

// ---------------- Utilidades ----------------
glm::vec2 centroTile(glm::ivec2 t) { return glm::vec2(t) + 0.5f; }

std::string formatarTempo(float s) {
    int t = std::max(0, (int)std::ceil(s));
    std::string seg = std::to_string(t % 60);
    return std::to_string(t / 60) + ":" + (seg.size() < 2 ? "0" : "") + seg;
}

void mostrarMensagem(const std::string &m, float duracao = 3.0f) {
    mensagem = m;
    tempoMensagem = duracao;
}

void desenharQuad(GLuint tex, glm::vec4 uvRect, glm::vec2 centro, glm::vec2 tamanho, int flags = 0,
                  glm::vec4 cor = glm::vec4(1.0f), float angulo = 0.0f) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(centro, 0.0f));
    model = glm::rotate(model, glm::radians(angulo), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(tamanho, 1.0f));
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(uvRectLoc, 1, glm::value_ptr(uvRect));
    glUniform1i(flagsLoc, flags);
    glUniform4fv(corLoc, 1, glm::value_ptr(cor));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Desenha o tile "indice" do atlas (ou só o sub-retângulo "sub" dele)
void desenharTile(int indice, glm::vec2 centro, glm::vec2 tamanho, int flags = 0, glm::vec4 cor = glm::vec4(1.0f),
                  float angulo = 0.0f, glm::vec4 sub = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)) {
    float col = indice % COLUNAS_ATLAS, lin = indice / COLUNAS_ATLAS;
    glm::vec4 uv((col + sub.x) / COLUNAS_ATLAS, (lin + sub.y) / LINHAS_ATLAS, sub.z / COLUNAS_ATLAS,
                 sub.w / LINHAS_ATLAS);
    desenharQuad(texAtlas, uv, centro, tamanho, flags, cor, angulo);
}

void desenharRetangulo(glm::vec2 centro, glm::vec2 tamanho, glm::vec4 cor) {
    glUniform1i(solidoLoc, 1);
    desenharQuad(texAtlas, glm::vec4(0.0f), centro, tamanho, 0, cor);
    glUniform1i(solidoLoc, 0);
}

float larguraTexto(const std::string &s, float escala) { return s.size() * 6.0f * escala - escala; }

// Texto com a fonte bitmap; "centro" é o meio da linha. Desenha uma sombra antes.
void desenharTexto(const std::string &s, glm::vec2 centro, float escala, glm::vec4 cor) {
    float x = centro.x - larguraTexto(s, escala) / 2.0f, y = centro.y - 3.5f * escala;
    for (int passada = 0; passada < 2; passada++) {
        glm::vec2 desloc = passada == 0 ? glm::vec2(escala) : glm::vec2(0.0f);
        glm::vec4 c = passada == 0 ? glm::vec4(0.0f, 0.0f, 0.0f, 0.7f * cor.a) : cor;
        for (size_t i = 0; i < s.size(); i++) {
            size_t k = CARACTERES.find((char)std::toupper((unsigned char)s[i]));
            if (k == std::string::npos) k = CARACTERES.size() - 1;
            glm::vec4 uv(k * 6.0f / larguraFonte, 0.0f, 5.0f / larguraFonte, 1.0f);
            glm::vec2 p(x + i * 6.0f * escala + 2.5f * escala, y + 3.5f * escala);
            desenharQuad(texFonte, uv, p + desloc, glm::vec2(5.0f, 7.0f) * escala, 0, c);
        }
    }
}

// ---------------- Carregamento ----------------
GLuint criarTextura(const unsigned char *dados, int w, int h) {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // pixel art: sem suavização
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

GLuint carregarTextura(const std::string &caminho) {
    int w, h, canais;
    unsigned char *dados = stbi_load(caminho.c_str(), &w, &h, &canais, 4);
    if (!dados) {
        std::cerr << "Falha ao carregar textura: " << caminho << std::endl;
        return 0;
    }
    GLuint id = criarTextura(dados, w, h);
    stbi_image_free(dados);
    return id;
}

// Cada glifo ocupa 6 colunas (5 + 1 de espaço) numa textura de 7 linhas
void criarFonte() {
    int n = CARACTERES.size();
    larguraFonte = n * 6;
    std::vector<unsigned char> px(larguraFonte * 7 * 4, 0);
    for (int g = 0; g < n; g++)
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (FONTE[g][y][x] == '#') {
                    unsigned char *p = &px[((y * larguraFonte) + g * 6 + x) * 4];
                    p[0] = p[1] = p[2] = p[3] = 255;
                }
    texFonte = criarTextura(px.data(), larguraFonte, 7);
}

bool carregarMapa(const std::string &caminho) {
    std::ifstream arq(caminho);
    if (!arq) {
        std::cerr << "Falha ao abrir o mapa: " << caminho << std::endl;
        return false;
    }
    std::string linha, secao;
    while (std::getline(arq, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();
        if (linha.empty()) continue;
        if (linha[0] == '[') { secao = linha; continue; }
        if (secao == "[mapa]") {
            mapa.linhas.push_back(linha);
        } else if (secao == "[inimigos]" && linha[0] != '#') {
            std::istringstream ss(linha);
            std::string tipo, modo;
            Inimigo e{};
            ss >> tipo >> modo >> e.vel;
            float x, y;
            while (ss >> x >> y) e.pontos.push_back({x + 0.5f, y + 0.5f});
            if (e.pontos.empty()) continue;
            e.tipo = tipo == "morcego" ? MORCEGO : RATO;
            e.modo = modo == "circuito" ? CIRCUITO : VAIVEM;
            e.pos = e.pontos[0];
            e.alvo = e.pontos.size() > 1 ? 1 : 0;
            e.fase = inimigosIniciais.size() * 1.7f;
            inimigosIniciais.push_back(e);
        }
    }
    mapa.altura = mapa.linhas.size();
    for (auto &l : mapa.linhas) mapa.largura = std::max(mapa.largura, (int)l.size());
    for (auto &l : mapa.linhas) l.resize(mapa.largura, '#');

    // Objetos marcados no mapa: viram entidades e o tile vira chão
    for (int y = 0; y < mapa.altura; y++)
        for (int x = 0; x < mapa.largura; x++) {
            char &c = mapa.linhas[y][x];
            if (c == 'P') { jogador.inicio = {x, y}; c = '.'; }
            else if (c == 'p') { pocoes.push_back({{x, y}, false}); c = '.'; }
            else if (c == 'C') posBau = {x, y};
            else if (c == 'E') posPorta = {x, y};
            else if (c == '^') espinhos.push_back({x, y});
        }
    return mapa.altura > 0;
}

// Escolhe o tile de um quadrante do topo da parede. "a" = vizinho vertical é buraco,
// "b" = vizinho horizontal é buraco, "diag" = vizinho diagonal é buraco.
int quadrante(bool a, bool b, bool diag, int quina, int bordaVert, int bordaHoriz, int canto) {
    if (a && b) return quina;
    if (a) return bordaVert;
    if (b) return bordaHoriz;
    if (diag) return canto;
    return Tile::TOPO;
}

// Monta a camada estática (autotile das paredes, sombras do chão e decoração)
void montarCamadaBase() {
    using namespace Tile;
    camadaBase.clear();
    auto buraco = [](int x, int y) { return !mapa.topo(x, y); };
    for (int y = 0; y < mapa.altura; y++)
        for (int x = 0; x < mapa.largura; x++) {
            char c = mapa.em(x, y);
            glm::vec2 canto(x * T, y * T), centro = canto + T / 2.0f;
            if (mapa.topo(x, y)) {
                bool n = buraco(x, y - 1), s = buraco(x, y + 1), l = buraco(x + 1, y), o = buraco(x - 1, y);
                int q[4] = {quadrante(n, o, buraco(x - 1, y - 1), QUINA_NO, BORDA_N, BORDA_O, CANTO_NO),
                            quadrante(n, l, buraco(x + 1, y - 1), QUINA_NE, BORDA_N, BORDA_L, CANTO_NE),
                            quadrante(s, o, buraco(x - 1, y + 1), QUINA_SO, BORDA_S, BORDA_O, CANTO_SO),
                            quadrante(s, l, buraco(x + 1, y + 1), QUINA_SE, BORDA_S, BORDA_L, CANTO_SE)};
                for (int i = 0; i < 4; i++) {
                    glm::vec2 qd(i % 2 * 0.5f, i / 2 * 0.5f);
                    camadaBase.push_back({q[i], canto + (qd + 0.25f) * T, glm::vec2(T / 2.0f),
                                          glm::vec4(qd, 0.5f, 0.5f), 0});
                }
                continue;
            }
            if (mapa.face(x, y)) {
                bool e = !mapa.parede(x - 1, y), d = !mapa.parede(x + 1, y);
                int t = e && d ? FACE_AMBOS : e ? FACE_ESQ : d ? FACE_DIR : FACE;
                if (c == 'G') t = GRADE;
                if (c == 'T') t = TOCHA;
                camadaBase.push_back({t, centro, glm::vec2(T), glm::vec4(0, 0, 1, 1), 0});
                continue;
            }
            // Chão: sombra embaixo das faces (em cima) e ao lado das paredes (à esquerda)
            bool n = mapa.parede(x, y - 1), o = mapa.parede(x - 1, y), no = mapa.parede(x - 1, y - 1);
            bool pintado = ((x * 73856093) ^ (y * 19349663)) % 5 == 0;
            int t = pintado ? CHAO2 : CHAO, flags = 0;
            if (n && o) { t = SOMBRA_CANTO; flags = ESPELHAR; }
            else if (n) t = pintado ? SOMBRA2 : SOMBRA;
            else if (o) { t = pintado ? SOMBRA2 : SOMBRA; flags = TRANSPOR; }
            else if (no) t = SOMBRA_PONTA;
            camadaBase.push_back({t, centro, glm::vec2(T), glm::vec4(0, 0, 1, 1), flags});

            int objeto = c == '+' ? CRUZ : c == 'x' ? LAPIDE
                       : c == 't' ? MESA : c == 's' ? BANCO : c == 'a' ? BIGORNA : c == 'b' ? BARRIL : -1;
            if (objeto >= 0) camadaBase.push_back({objeto, centro, glm::vec2(T), glm::vec4(0, 0, 1, 1), 0});
        }
}

// ---------------- Lógica ----------------
void reiniciar(bool comecar) {
    inimigos = inimigosIniciais;
    for (auto &p : pocoes) p.pega = false;
    jogador.tile = checkpoint = jogador.inicio;
    jogador.pos = centroTile(jogador.tile);
    jogador.movendo = false;
    jogador.invulneravel = 0.0f;
    vidas = VIDAS;
    temChave = false;
    aberturaPorta = tempoChave = flashDano = 0.0f;
    tempoRestante = TEMPO_LIMITE;
    estado = comecar ? JOGANDO : INICIO;
    tempoEstado = 0.0f;
    pausado = false;
    if (comecar) mostrarMensagem("ENCONTRE A CHAVE NO BAU E FUJA PELA PORTA!", 4.0f);
}

void terminar(Estado e, const std::string &motivo = "") {
    estado = e;
    tempoEstado = 0.0f;
    motivoDerrota = motivo;
    tempoMensagem = 0.0f;
}

bool espinhoAlto(glm::ivec2 t, float *fase = nullptr) {
    float f = std::fmod(tempo + t.x * 0.2f, PERIODO_ESPINHOS);
    if (fase) *fase = f;
    return f < ESPINHOS_ALTO;
}

void levarDano() {
    vidas--;
    flashDano = 1.0f;
    if (vidas <= 0) { terminar(DERROTA, "VOCE FOI DERROTADO NA MASMORRA"); return; }
    jogador.tile = checkpoint;
    jogador.pos = centroTile(checkpoint);
    jogador.movendo = false;
    jogador.invulneravel = INVULNERAVEL;
    mostrarMensagem("AI! VOCE PERDEU UMA VIDA", 2.0f);
}

bool andavel(glm::ivec2 t) {
    char c = mapa.em(t.x, t.y);
    return c == '.' || c == '^' || (c == 'E' && aberturaPorta >= 1.0f);
}

bool tentarMover(glm::ivec2 d) {
    if (d.x != 0) jogador.esquerda = d.x < 0;
    glm::ivec2 alvo = jogador.tile + d;
    if (alvo == posBau && !temChave) {
        temChave = true;
        checkpoint = jogador.tile;
        mostrarMensagem("VOCE PEGOU A CHAVE! A PORTA SE ABRIU");
        return false;
    }
    if (mapa.em(alvo.x, alvo.y) == 'E' && !temChave) {
        mostrarMensagem("A PORTA ESTA TRANCADA. PROCURE A CHAVE!", 2.5f);
        return false;
    }
    if (!andavel(alvo)) return false;
    jogador.origem = jogador.pos;
    jogador.tile = alvo;
    jogador.movendo = true;
    jogador.progresso = 0.0f;
    return true;
}

void chegouNoTile() {
    if (jogador.tile == posPorta) { terminar(VITORIA); return; }
    for (auto &p : pocoes)
        if (!p.pega && p.tile == jogador.tile && vidas < VIDAS) {
            p.pega = true;
            vidas++;
            mostrarMensagem("POCAO! +1 VIDA", 2.0f);
        }
}

glm::ivec2 lerDirecao(GLFWwindow *w) {
    auto k = [w](int a, int b) { return glfwGetKey(w, a) == GLFW_PRESS || glfwGetKey(w, b) == GLFW_PRESS; };
    if (k(GLFW_KEY_W, GLFW_KEY_UP)) return {0, -1};
    if (k(GLFW_KEY_S, GLFW_KEY_DOWN)) return {0, 1};
    if (k(GLFW_KEY_A, GLFW_KEY_LEFT)) return {-1, 0};
    if (k(GLFW_KEY_D, GLFW_KEY_RIGHT)) return {1, 0};
    return {0, 0};
}

// Movimento de tile em tile, interpolado; se a tecla continua pressionada, emenda o próximo passo
void atualizarJogador(float dt, GLFWwindow *w) {
    if (jogador.invulneravel > 0.0f) jogador.invulneravel -= dt;
    if (jogador.movendo) {
        jogador.progresso += VEL_JOGADOR * dt;
        if (jogador.progresso >= 1.0f) {
            float sobra = jogador.progresso - 1.0f;
            jogador.movendo = false;
            jogador.pos = centroTile(jogador.tile);
            chegouNoTile();
            glm::ivec2 d = lerDirecao(w);
            if (estado == JOGANDO && d != glm::ivec2(0) && tentarMover(d)) jogador.progresso = sobra;
        }
    } else {
        glm::ivec2 d = lerDirecao(w);
        if (d != glm::ivec2(0)) tentarMover(d);
    }
    if (jogador.movendo) jogador.pos = glm::mix(jogador.origem, centroTile(jogador.tile), jogador.progresso);
}

void moverInimigo(Inimigo &e, float dt) {
    glm::vec2 alvo = e.pontos[e.alvo];
    glm::vec2 d = alvo - e.pos;
    float dist = glm::length(d), passo = e.vel * dt;
    if (std::abs(d.x) > 0.01f) e.esquerda = d.x < 0.0f;
    if (dist > passo) { e.pos += d / dist * passo; return; }
    e.pos = alvo;
    int n = e.pontos.size();
    if (e.modo == CIRCUITO) e.alvo = (e.alvo + 1) % n;
    else if (n > 1) {
        if (e.alvo + e.sentido >= n || e.alvo + e.sentido < 0) e.sentido = -e.sentido;
        e.alvo += e.sentido;
    }
}

float raioInimigo(const Inimigo &e) { return e.tipo == MORCEGO ? 0.5f : 0.6f; }

void atualizar(float dt, GLFWwindow *w) {
    tempo += dt;
    tempoEstado += dt;
    tempoMensagem -= dt;
    flashDano = std::max(0.0f, flashDano - dt * 2.5f);
    if (temChave) {
        aberturaPorta = std::min(1.0f, aberturaPorta + dt / 0.8f);
        tempoChave += dt;
    }
    for (auto &e : inimigos) moverInimigo(e, dt);
    if (estado != JOGANDO) return;

    tempoRestante -= dt;
    if (tempoRestante <= 0.0f) { terminar(DERROTA, "O TEMPO ACABOU"); return; }
    atualizarJogador(dt, w);
    if (estado != JOGANDO || jogador.invulneravel > 0.0f) return;

    // Colisões: espinhos levantados no tile em que o jogador está e contato com inimigos
    glm::ivec2 t(glm::floor(jogador.pos));
    if (mapa.em(t.x, t.y) == '^' && espinhoAlto(t)) { levarDano(); return; }
    for (auto &e : inimigos)
        if (glm::length(e.pos - jogador.pos) < raioInimigo(e)) { levarDano(); return; }
}

// ---------------- Desenho ----------------
// Luzes da cena em pixels: (x, y, raio, intensidade) e cor. Só o jogador e os itens iluminam.
void enviarLuzes() {
    std::vector<glm::vec4> luzes;
    std::vector<glm::vec3> cores;
    auto luz = [&](glm::vec2 p, float raio, float intens, glm::vec3 cor) {
        if ((int)luzes.size() >= MAX_LUZES) return;
        luzes.push_back({p * T, raio * T, intens});
        cores.push_back(cor);
    };
    luz(jogador.pos, 4.2f, 1.0f, {1.0f, 0.85f, 0.6f});
    if (!temChave) luz(centroTile(posBau), 1.8f, 0.6f + 0.3f * std::sin(tempo * 3.0f), {1.0f, 0.85f, 0.3f});
    for (auto &p : pocoes)
        if (!p.pega) luz(centroTile(p.tile), 1.3f, 0.6f, {1.0f, 0.3f, 0.3f});
    glUniform1f(ambienteLoc, AMBIENTE);
    glUniform1i(numLuzesLoc, (int)luzes.size());
    glUniform4fv(luzesLoc, (GLsizei)luzes.size(), glm::value_ptr(luzes[0]));
    glUniform3fv(coresLuzLoc, (GLsizei)cores.size(), glm::value_ptr(cores[0]));
}

void desenharInimigo(const Inimigo &e) {
    glm::vec2 p = e.pos * T, tam(T);
    int flags = e.esquerda ? ESPELHAR : 0;
    glm::vec4 cor(1.0f);
    int tile = Tile::SPR_RATO;
    switch (e.tipo) {
    case RATO: p.y -= std::abs(std::sin(tempo * 14.0f + e.fase)) * 3.0f; break;
    case MORCEGO: // bate as asas (escala) e oscila na vertical
        tile = Tile::SPR_MORCEGO;
        tam.x *= 0.8f + 0.2f * std::abs(std::sin(tempo * 12.0f + e.fase));
        p.y += std::sin(tempo * 4.0f + e.fase) * 0.15f * T - 0.2f * T;
        break;
    }
    desenharTile(tile, p, tam, flags, cor);
}

void desenharMundo() {
    using namespace Tile;
    for (auto &p : camadaBase) desenharTile(p.tile, p.centro, p.tamanho, p.flags, glm::vec4(1.0f), 0.0f, p.sub);

    // Porta: animação de abertura em 4 quadros
    int quadro = std::min(3, (int)(aberturaPorta * 4.0f));
    desenharTile(PORTA[quadro], centroTile(posPorta) * T, glm::vec2(T));

    // Espinhos: baixos (menores e escuros), piscam antes de subir, altos (tamanho cheio)
    for (auto &s : espinhos) {
        float fase;
        bool alto = espinhoAlto(s, &fase);
        bool aviso = fase > PERIODO_ESPINHOS - AVISO_ESPINHOS && std::fmod(tempo, 0.16f) < 0.08f;
        glm::vec4 cor = alto ? glm::vec4(1.0f) : aviso ? glm::vec4(1.0f, 0.55f, 0.5f, 1.0f) : glm::vec4(0.45f, 0.4f, 0.4f, 1.0f);
        desenharTile(ESPINHOS, centroTile(s) * T, glm::vec2(alto ? T : 2.0f * TILE), 0, cor);
    }

    // Baú, chave subindo e poções
    desenharTile(temChave ? BAU_ABERTO : BAU, centroTile(posBau) * T, glm::vec2(T));
    for (auto &p : pocoes)
        if (!p.pega) desenharTile(POCAO, (centroTile(p.tile) + glm::vec2(0.0f, 0.08f * std::sin(tempo * 3.0f))) * T, glm::vec2(T));

    // Inimigos de chão, jogador e inimigos voadores (na frente)
    for (auto &e : inimigos)
        if (e.tipo != MORCEGO) desenharInimigo(e);
    bool pisca = jogador.invulneravel > 0.0f && std::fmod(tempo, 0.16f) < 0.08f;
    if (!pisca && !(estado == VITORIA && tempoEstado > 0.4f)) {
        float pulo = jogador.movendo ? std::sin(jogador.progresso * 3.14159f) * 4.0f : 0.0f;
        float alpha = estado == VITORIA ? 1.0f - tempoEstado / 0.4f : 1.0f;
        desenharTile(CAVALEIRO, jogador.pos * T - glm::vec2(0.0f, pulo), glm::vec2(T), jogador.esquerda ? ESPELHAR : 0,
                     glm::vec4(1.0f, 1.0f, 1.0f, alpha));
    }
    for (auto &e : inimigos)
        if (e.tipo == MORCEGO) desenharInimigo(e);
}

void desenharGrade() {
    glm::vec4 cor(1.0f, 1.0f, 1.0f, 0.18f);
    for (int x = 0; x <= mapa.largura; x++) desenharRetangulo({x * T, HEIGHT / 2.0f}, {1.0f, (float)HEIGHT}, cor);
    for (int y = 0; y <= mapa.altura; y++) desenharRetangulo({WIDTH / 2.0f, y * T}, {(float)WIDTH, 1.0f}, cor);
}

void desenharHUD() {
    glm::vec4 branco(1.0f), dourado(1.0f, 0.82f, 0.3f, 1.0f), cinza(0.45f, 0.45f, 0.45f, 1.0f);
    desenharRetangulo({WIDTH / 2.0f, T / 2.0f}, {(float)WIDTH, T}, {0.06f, 0.03f, 0.06f, 0.75f});

    for (int i = 0; i < VIDAS; i++)
        desenharTile(Tile::CAVALEIRO, {30.0f + i * 42.0f, T / 2.0f}, glm::vec2(2.0f * TILE), 0,
                     i < vidas ? glm::vec4(1.0f) : glm::vec4(0.25f, 0.25f, 0.25f, 0.6f));

    std::string textoChave = temChave ? "PORTA ABERTA" : "CHAVE";
    float xChave = WIDTH / 2.0f;
    desenharTexto("~", {xChave - larguraTexto(textoChave, 3.0f) / 2.0f - 24.0f, T / 2.0f}, 4.0f, temChave ? dourado : cinza);
    desenharTexto(textoChave, {xChave, T / 2.0f}, 3.0f, temChave ? dourado : cinza);

    std::string textoTempo = "TEMPO " + formatarTempo(tempoRestante);
    bool urgente = tempoRestante < 30.0f && std::fmod(tempo, 0.6f) < 0.3f;
    desenharTexto(textoTempo, {WIDTH - 20.0f - larguraTexto(textoTempo, 3.0f) / 2.0f, T / 2.0f}, 3.0f,
                  urgente ? glm::vec4(1.0f, 0.3f, 0.3f, 1.0f) : branco);

    if (tempoMensagem > 0.0f && estado == JOGANDO) {
        float a = std::min(1.0f, tempoMensagem * 2.0f);
        desenharRetangulo({WIDTH / 2.0f, HEIGHT - T / 2.0f}, {(float)WIDTH, T}, {0.06f, 0.03f, 0.06f, 0.75f * a});
        desenharTexto(mensagem, {WIDTH / 2.0f, HEIGHT - T / 2.0f}, 3.0f, {1.0f, 1.0f, 1.0f, a});
    }
}

void desenharTelas() {
    glm::vec4 branco(1.0f), dourado(1.0f, 0.82f, 0.3f, 1.0f);
    glm::vec2 c(WIDTH / 2.0f, HEIGHT / 2.0f);
    bool piscar = std::fmod(tempo, 1.0f) < 0.6f;
    if (estado == INICIO) {
        desenharRetangulo(c, {(float)WIDTH, (float)HEIGHT}, {0.0f, 0.0f, 0.0f, 0.65f});
        desenharTexto("DUNGEON ESCAPE", c - glm::vec2(0.0f, 170.0f), 9.0f, dourado);
        desenharTexto("ABRA O BAU PARA PEGAR A CHAVE E FUJA PELA PORTA", c - glm::vec2(0.0f, 60.0f), 3.0f, branco);
        desenharTexto("CUIDADO COM OS ESPINHOS E OS MONSTROS", c - glm::vec2(0.0f, 25.0f), 3.0f, branco);
        desenharTexto("3 VIDAS - " + formatarTempo(TEMPO_LIMITE) + " DE TEMPO", c + glm::vec2(0.0f, 10.0f), 3.0f, branco);
        desenharTexto("WASD/SETAS MOVER  P PAUSA  R REINICIA", c + glm::vec2(0.0f, 70.0f), 3.0f, {0.7f, 0.7f, 0.7f, 1.0f});
        desenharTexto("G GRADE  ESC SAIR", c + glm::vec2(0.0f, 105.0f), 3.0f, {0.7f, 0.7f, 0.7f, 1.0f});
        if (piscar) desenharTexto("PRESSIONE ENTER PARA COMECAR", c + glm::vec2(0.0f, 180.0f), 4.0f, dourado);
    } else if (estado == VITORIA || estado == DERROTA) {
        float a = std::min(1.0f, tempoEstado * 2.0f);
        desenharRetangulo(c, {(float)WIDTH, (float)HEIGHT}, {0.0f, 0.0f, 0.0f, 0.7f * a});
        if (estado == VITORIA) {
            desenharTexto("VOCE ESCAPOU!", c - glm::vec2(0.0f, 110.0f), 9.0f, {0.4f, 1.0f, 0.5f, a});
            desenharTexto("TEMPO RESTANTE " + formatarTempo(tempoRestante) + "   VIDAS " + std::to_string(vidas),
                          c, 4.0f, glm::vec4(1.0f, 1.0f, 1.0f, a));
        } else {
            desenharTexto("GAME OVER", c - glm::vec2(0.0f, 110.0f), 9.0f, {1.0f, 0.3f, 0.3f, a});
            desenharTexto(motivoDerrota, c, 4.0f, glm::vec4(1.0f, 1.0f, 1.0f, a));
        }
        if (piscar && tempoEstado > 0.8f)
            desenharTexto("PRESSIONE R PARA JOGAR DE NOVO", c + glm::vec2(0.0f, 120.0f), 4.0f, dourado);
    }
    if (pausado) {
        desenharRetangulo(c, {(float)WIDTH, (float)HEIGHT}, {0.0f, 0.0f, 0.0f, 0.5f});
        desenharTexto("PAUSADO", c, 8.0f, branco);
    }
}

void desenharCena() {
    glUniform1i(iluminarLoc, 1); // só o mundo é iluminado; grade, HUD e telas não
    enviarLuzes();
    desenharMundo();
    glUniform1i(iluminarLoc, 0);
    if (mostrarGrade) desenharGrade();
    if (flashDano > 0.0f) desenharRetangulo({WIDTH / 2.0f, HEIGHT / 2.0f}, {(float)WIDTH, (float)HEIGHT}, {0.8f, 0.0f, 0.0f, 0.4f * flashDano});
    if (estado == JOGANDO && temChave && tempoChave < 1.5f) { // chave saindo do baú
        float k = tempoChave / 1.5f;
        desenharTexto("~", (centroTile(posBau) - glm::vec2(0.0f, 0.4f + 1.2f * k)) * T, 5.0f, {1.0f, 0.82f, 0.3f, 1.0f - k * k});
    }
    desenharHUD();
    desenharTelas();
}

void atualizarTitulo(GLFWwindow *window) {
    static std::string anterior;
    std::string titulo = "M6 - Dungeon Escape | Vidas: " + std::to_string(vidas) + " | Tempo: " +
                         formatarTempo(tempoRestante) + (temChave ? " | Chave: SIM" : " | Chave: NAO");
    if (pausado) titulo += " | PAUSADO";
    if (titulo != anterior) glfwSetWindowTitle(window, titulo.c_str());
    anterior = titulo;
}

// ---------------- Entrada ----------------
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) && estado != JOGANDO) reiniciar(true);
    if (key == GLFW_KEY_R) reiniciar(true);
    if (key == GLFW_KEY_P && estado == JOGANDO) pausado = !pausado;
    if (key == GLFW_KEY_G) mostrarGrade = !mostrarGrade;
}

void framebuffer_size_callback(GLFWwindow *, int width, int height) { glViewport(0, 0, width, height); }

// ---------------- Inicialização OpenGL ----------------
GLuint compilarShader(GLenum tipo, const GLchar *fonte) {
    GLuint shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &fonte, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Erro ao compilar shader:\n" << log << std::endl;
    }
    return shader;
}

GLuint setupShader() {
    GLuint vs = compilarShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compilarShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Erro ao linkar programa:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// Quadrado unitário centralizado na origem
void setupGeometria() {
    GLfloat vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    GLuint VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

int main() {
    if (!carregarMapa(MAPA_ARQUIVO)) return -1;
    if (posBau.x < 0 || posPorta.x < 0) {
        std::cerr << "O mapa precisa de um baú (C) e de uma porta (E)" << std::endl;
        return -1;
    }
    WIDTH = mapa.largura * T;
    HEIGHT = mapa.altura * T;

    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "M6 - Dungeon Escape", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    setupGeometria();
    glUseProgram(shaderID);
    modelLoc = glGetUniformLocation(shaderID, "model");
    uvRectLoc = glGetUniformLocation(shaderID, "uvRect");
    flagsLoc = glGetUniformLocation(shaderID, "flags");
    corLoc = glGetUniformLocation(shaderID, "cor");
    solidoLoc = glGetUniformLocation(shaderID, "solido");
    iluminarLoc = glGetUniformLocation(shaderID, "iluminar");
    ambienteLoc = glGetUniformLocation(shaderID, "ambiente");
    numLuzesLoc = glGetUniformLocation(shaderID, "numLuzes");
    luzesLoc = glGetUniformLocation(shaderID, "luzes");
    coresLuzLoc = glGetUniformLocation(shaderID, "coresLuz");
    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);
    glUniform1i(solidoLoc, 0);
    glActiveTexture(GL_TEXTURE0);

    // Projeção ortográfica em pixels, origem no canto superior esquerdo (y para baixo)
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    texAtlas = carregarTextura(std::string(ASSETS_DIR) + "Tilemap/tilemap_packed.png");
    criarFonte();
    montarCamadaBase();
    reiniciar(false);

    std::cout << "\nDungeon Escape - abra o bau, pegue a chave e fuja pela porta!\n"
                 "  WASD / setas: mover | ENTER: comecar | P: pausa | R: reinicia\n"
                 "  G: grade do tilemap | ESC: sai\n";

    double ultimoTempo = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double agora = glfwGetTime();
        float dt = std::min((float)(agora - ultimoTempo), 1.0f / 30.0f);
        ultimoTempo = agora;
        if (!pausado) atualizar(dt, window);
        atualizarTitulo(window);

        glClearColor(0.05f, 0.03f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        desenharCena();
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &texAtlas);
    glDeleteTextures(1, &texFonte);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
