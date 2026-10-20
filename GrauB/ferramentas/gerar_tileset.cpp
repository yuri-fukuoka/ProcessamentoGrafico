/*
 * Gerador do tileset isométrico (Grau B)
 *
 * Os tiles do pack 0x72 DungeonTileset II são desenhados de cima (top-down, 16x16).
 * Este programa "assa" cada um deles em um tile isométrico diamond 2:1 e grava todos
 * lado a lado em tilesetIso.png, no formato esperado pelo map.txt (N tiles de 32x32).
 *
 * Cada célula de 32x32 tem a "pegada" do tile (losango de 32x16) na metade de baixo:
 *
 *        (16,16)                     topo do bloco (só paredes): o mesmo losango 16 px acima
 *   (0,24)      (32,24)              face esquerda: entre (0,8)-(16,16) e (0,24)-(16,32)
 *        (16,32)                     face direita:  entre (16,16)-(32,8) e (16,32)-(32,24)
 *
 * Para cada pixel do losango, a transformação inversa da projeção isométrica diz de
 * qual pixel do tile original ele vem:
 *     tela -> tile:  a = (x/16 + y/8) / 2   (coluna)      b = (y/8 - x/16) / 2   (linha)
 * Paredes viram blocos com altura de 16 px: topo + duas faces com sombreamento diferente,
 * para dar a impressão de volume (a luz vem da esquerda).
 *
 * Uso: ./GrauB_GerarTileset   (os caminhos de entrada e saída vêm do CMake)
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const int CEL = 32;    // tamanho da célula no tileset
const int ALTURA = 16; // altura dos blocos de parede

struct Imagem {
    int w = 0, h = 0;
    std::vector<uint8_t> px; // RGBA
    uint8_t *em(int x, int y) { return &px[(y * w + x) * 4]; }
};

Imagem carregar(const std::string &nome) {
    Imagem img;
    int n;
    std::string caminho = std::string(FRAMES_DIR) + nome;
    uint8_t *d = stbi_load(caminho.c_str(), &img.w, &img.h, &n, 4);
    if (!d) {
        fprintf(stderr, "Falha ao carregar %s\n", caminho.c_str());
        exit(1);
    }
    img.px.assign(d, d + img.w * img.h * 4);
    stbi_image_free(d);
    return img;
}

// Sobrepõe "cima" em "base" (composição alfa simples), por exemplo o botão sobre o chão
Imagem compor(Imagem base, Imagem cima) {
    for (int i = 0; i < base.w * base.h; i++) {
        float a = cima.px[i * 4 + 3] / 255.0f;
        for (int k = 0; k < 3; k++) base.px[i * 4 + k] = (uint8_t)(base.px[i * 4 + k] * (1 - a) + cima.px[i * 4 + k] * a);
    }
    return base;
}

void pintar(Imagem &dst, int x, int y, const uint8_t *cor, float luz = 1.0f) {
    uint8_t *p = dst.em(x, y);
    for (int k = 0; k < 3; k++) p[k] = (uint8_t)std::min(255.0f, cor[k] * luz);
    p[3] = cor[3];
}

// Losango do tile (pegada) com o canto de cima em (16, topo)
void desenharLosango(Imagem &cel, Imagem &tex, int topo, float luz) {
    for (int y = topo; y < topo + 16; y++)
        for (int x = 0; x < CEL; x++) {
            float dx = x + 0.5f - 16.0f, dy = y + 0.5f - topo;
            float a = (dx / 16.0f + dy / 8.0f) / 2.0f, b = (dy / 8.0f - dx / 16.0f) / 2.0f;
            if (a < 0 || a >= 1 || b < 0 || b >= 1) continue;
            pintar(cel, x, y, tex.em((int)(a * tex.w), (int)(b * tex.h)), luz);
        }
}

// Faces laterais do bloco: a textura é "colada" na vertical, seguindo a aresta de baixo do topo
void desenharFaces(Imagem &cel, Imagem &tex) {
    for (int x = 0; x < CEL; x++) {
        bool esquerda = x < 16;
        float u = esquerda ? (x + 0.5f) / 16.0f : (x + 0.5f - 16.0f) / 16.0f;
        int yTopo = esquerda ? 8 + x / 2 : 16 - (x - 16) / 2;
        for (int h = 0; h < ALTURA; h++) {
            int y = yTopo + h;
            if (y >= CEL) break;
            pintar(cel, x, y, tex.em((int)(u * tex.w), h * tex.h / ALTURA), esquerda ? 0.85f : 0.6f);
        }
    }
}

Imagem celulaVazia() {
    Imagem c;
    c.w = c.h = CEL;
    c.px.assign(CEL * CEL * 4, 0);
    return c;
}

Imagem chao(Imagem tex) {
    Imagem c = celulaVazia();
    desenharLosango(c, tex, 16, 1.0f);
    return c;
}

Imagem bloco(Imagem face, Imagem topo) {
    Imagem c = celulaVazia();
    desenharFaces(c, face);
    desenharLosango(c, topo, 0, 0.75f);
    return c;
}

// Só a borda do losango (pixels do losango que têm algum vizinho fora dele)
Imagem contorno() {
    Imagem cheio = celulaVazia(), c = celulaVazia();
    Imagem branco;
    branco.w = branco.h = 1;
    branco.px = {255, 255, 255, 255};
    desenharLosango(cheio, branco, 16, 1.0f);
    auto dentro = [&](int x, int y) { return x >= 0 && y >= 0 && x < CEL && y < CEL && cheio.em(x, y)[3] > 0; };
    for (int y = 0; y < CEL; y++)
        for (int x = 0; x < CEL; x++)
            if (dentro(x, y) && !(dentro(x - 1, y) && dentro(x + 1, y) && dentro(x, y - 1) && dentro(x, y + 1)))
                pintar(c, x, y, branco.px.data());
    return c;
}

// ---------------- PNG (sem compressão: blocos "stored" do deflate) ----------------
uint32_t crc32(const uint8_t *d, size_t n, uint32_t c = 0xffffffffu) {
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return c;
}

void gravarPNG(const std::string &caminho, Imagem &img) {
    std::vector<uint8_t> bruto;
    for (int y = 0; y < img.h; y++) {
        bruto.push_back(0);
        bruto.insert(bruto.end(), img.em(0, y), img.em(0, y) + img.w * 4);
    }
    std::vector<uint8_t> z = {0x78, 0x01};
    for (size_t i = 0; i < bruto.size(); i += 65535) {
        uint16_t n = (uint16_t)std::min<size_t>(65535, bruto.size() - i);
        z.push_back(i + n >= bruto.size());
        z.insert(z.end(), {(uint8_t)n, (uint8_t)(n >> 8), (uint8_t)~n, (uint8_t)(~n >> 8)});
        z.insert(z.end(), bruto.begin() + i, bruto.begin() + i + n);
    }
    uint32_t s1 = 1, s2 = 0;
    for (uint8_t b : bruto) { s1 = (s1 + b) % 65521; s2 = (s2 + s1) % 65521; }
    uint32_t adler = (s2 << 16) | s1;
    z.insert(z.end(), {(uint8_t)(adler >> 24), (uint8_t)(adler >> 16), (uint8_t)(adler >> 8), (uint8_t)adler});

    std::ofstream f(caminho, std::ios::binary);
    auto u32 = [&](uint32_t v) { uint8_t b[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v}; f.write((char *)b, 4); };
    auto chunk = [&](const char *tipo, const std::vector<uint8_t> &dados) {
        u32((uint32_t)dados.size());
        std::vector<uint8_t> t(tipo, tipo + 4);
        t.insert(t.end(), dados.begin(), dados.end());
        f.write((char *)t.data(), t.size());
        u32(crc32(t.data(), t.size()) ^ 0xffffffffu);
    };
    f.write("\x89PNG\r\n\x1a\n", 8);
    std::vector<uint8_t> ihdr = {(uint8_t)(img.w >> 24), (uint8_t)(img.w >> 16), (uint8_t)(img.w >> 8), (uint8_t)img.w,
                                 (uint8_t)(img.h >> 24), (uint8_t)(img.h >> 16), (uint8_t)(img.h >> 8), (uint8_t)img.h,
                                 8, 6, 0, 0, 0};
    chunk("IHDR", ihdr);
    chunk("IDAT", z);
    chunk("IEND", {});
}

int main() {
    Imagem chao1 = carregar("floor_1.png");
    Imagem paredeFace = carregar("wall_mid.png"), paredeTopo = carregar("floor_1.png");

    // A ordem aqui define os índices usados no map.txt
    std::vector<std::pair<std::string, Imagem>> tiles = {
        {"0  chao", chao(chao1)},
        {"1  chao (variacao)", chao(carregar("floor_2.png"))},
        {"2  chao (variacao)", chao(carregar("floor_3.png"))},
        {"3  chao (variacao)", chao(carregar("floor_4.png"))},
        {"4  buraco", chao(carregar("hole.png"))},
        {"5  placa levantada", chao(compor(chao1, carregar("button_red_up.png")))},
        {"6  placa abaixada", chao(compor(chao1, carregar("button_blue_down.png")))},
        {"7  espinhos (quadro 0)", chao(carregar("floor_spikes_anim_f0.png"))},
        {"8  espinhos (quadro 1)", chao(carregar("floor_spikes_anim_f1.png"))},
        {"9  espinhos (quadro 2)", chao(carregar("floor_spikes_anim_f2.png"))},
        {"10 espinhos (quadro 3)", chao(carregar("floor_spikes_anim_f3.png"))},
        {"11 saida fechada", chao(carregar("floor_ladder.png"))},
        {"12 escada (saida aberta)", chao(carregar("floor_stairs.png"))},
        {"13 parede", bloco(paredeFace, paredeTopo)},
        {"14 parede com estandarte", bloco(carregar("wall_banner_red.png"), paredeTopo)},
        {"15 parede com fonte", bloco(carregar("wall_fountain_mid_blue_anim_f0.png"), paredeTopo)},
        {"16 cursor (contorno do losango, usado pelo jogo)", contorno()},
    };

    Imagem saida;
    saida.w = CEL * (int)tiles.size();
    saida.h = CEL;
    saida.px.assign(saida.w * saida.h * 4, 0);
    for (size_t i = 0; i < tiles.size(); i++)
        for (int y = 0; y < CEL; y++)
            std::copy(tiles[i].second.em(0, y), tiles[i].second.em(0, y) + CEL * 4, saida.em((int)i * CEL, y));
    gravarPNG(SAIDA_TILESET, saida);

    printf("tilesetIso.png gerado: %zu tiles de %dx%d\n", tiles.size(), CEL, CEL);
    for (auto &t : tiles) printf("  %s\n", t.first.c_str());
    return 0;
}
