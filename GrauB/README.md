# Dungeon Escape Isométrico

Trabalho do **Grau B** da disciplina **Processamento Gráfico**: protótipo de jogo com **tilemap isométrico diamond**, em C++ com OpenGL 3.3.

## 👥 Integrantes do Grupo

- Yuri Fukuoka de Oliveira

## 🚀 Como executar

O projeto usa **CMake**. GLFW e GLM são baixados automaticamente, e o GLAD e o `stb_image` estão no repositório. Na raiz (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target GrauB
./build/GrauB
```

O tileset isométrico (`GrauB/tilesetIso.png`) já está no repositório. Para gerá-lo de novo a partir dos tiles originais:

```bash
cmake --build build --target GrauB_GerarTileset
./build/GrauB_GerarTileset
```

### Controles

| Tecla / mouse                    | Ação                                                     |
|----------------------------------|----------------------------------------------------------|
| `↑` `↓` `←` `→` ou `W` `S` `A` `D` | N, S, O, L                                             |
| Duas setas juntas (ex.: `↑`+`→`) | diagonais (NE, NO, SE, SO)                              |
| `Q` `E` `Z` `C`                  | diagonais direto: NO, NE, SO, SE                        |
| `ENTER`                          | começa (e recomeça na tela final)                       |
| `P`                              | pausa                                                    |
| `R`                              | reinicia                                                 |
| `V`                              | gira a visão 90° (mostra o que estava escondido atrás das paredes) |
| `H`                              | mostra / esconde a grade de losangos                    |
| `G`                              | modo invencível, para testes (não leva dano; aparece "INVENCIVEL" no título da janela) |
| Mouse                            | destaca o tile sob o cursor (linha, coluna e índice no título da janela) |
| `ESC`                            | sai                                                      |

## 💡 Sobre o Programa

Um cavaleiro está preso numa masmorra vista em **isométrico**:

- **Objetivo:** coletar as **8 moedas** e pisar nas **5 placas**. Quando as duas coisas estão completas, a saída fechada vira **escada**, e pisar nela **vence**.
- **Coleta:** moedas (objetivo) e poções vermelhas (+1 vida, quando falta alguma).
- **Evitação:** **buracos** e **espinhos levantados** tiram uma vida, assim como os **monstros** que patrulham as salas (esqueleto, goblin, diabinho e zumbi).
- **Derrota:** perder os 3 corações ou deixar o tempo (4:00) acabar.
- **Troca de tile:**
  - a **placa vermelha (5) vira azul (6)** ao ser pisada;
  - a **saída (11) vira escada (12)** quando o objetivo é cumprido;
  - os **espinhos** trocam de tile em sequência (7 → 8 → 9 → 10), subindo e descendo em onda.

O HUD mostra os corações, as moedas e placas que faltam e o tempo. O título da janela mostra a direção do último movimento e o tile sob o mouse.

### Arquivo de configuração (`map.txt`)

As três primeiras partes seguem exatamente o formato do enunciado:

```
tilesetIso.png 17 32 32      <- tileset, número de tiles, largura e altura de cada tile
18 18                        <- linhas e colunas
13 14 14 13 15 13 ...        <- matriz com o índice do tile em cada posição
```

Depois vêm as **regras dos tiles** e as **posições dos objetos**. Linhas com `#` são comentários e as coordenadas são sempre `linha coluna`:

| Linha do arquivo | Significado |
|---|---|
| `caminhaveis 0 1 2 ...` | índices onde o personagem pode pisar (paredes ficam de fora) |
| `perigosos 4 9 10` | pisar tira uma vida (buraco e espinhos levantados) |
| `troca 5 6` | ao pisar, o tile 5 vira 6 (pode haver várias linhas) |
| `saida 11 12` | com o objetivo cumprido, todos os tiles 11 viram 12 |
| `vitoria 12` | pisar neste tile vence |
| `animacao 6 7 7 7 7 8 9 10 ...` | quadros por segundo e sequência dos tiles animados (espinhos) |
| `jogador 1 1` | posição inicial |
| `moeda 3 16`, `pocao 8 8` | itens |
| `inimigo skelet vaivem 2.0 10 1 10 16` | sprite do pack, modo (`vaivem` ou `circuito`), velocidade em tiles/s e pontos do trajeto |

Dá para criar outra fase editando só esse arquivo, sem recompilar.

### Como funciona (decisões do projeto)

- **Projeção diamond.** O tile `(linha, coluna)` vai para a tela com:
  `x = origem.x + (coluna − linha) · L/2` e `y = origem.y + (coluna + linha) · A/2`,
  com o losango 2:1 (L = 64 px, A = 32 px na tela). Andar uma coluna desloca para a direita e para baixo, e andar uma linha desloca para a esquerda e para baixo. Isso forma o "diamante". A **inversa** (`telaParaIso`) resolve esse sistema para `linha` e `coluna`; é ela que descobre qual tile está sob o mouse.
- **Rotação da visão (`V`).** Antes de projetar, a posição `(linha, coluna)` passa por uma rotação de 90° do grid: `(l, c) → (c, L − l)`, aplicada 0 a 3 vezes, com `L` = número de linhas. O mapa e a lógica não mudam; só muda qual canto fica "no fundo" e, portanto, a ordem de desenho. Assim aparece o que ficava escondido atrás das paredes da frente. O mouse e o teclado desfazem a rotação: o tile sob o cursor é "desgirado" de volta para o mapa, e "para cima" continua sendo para cima na tela.
- **8 direções no diamond.** As direções da tela não são as do grid. **N** (subir na tela) é `(linha−1, coluna−1)`, **L** é `(linha−1, coluna+1)`, e as diagonais da tela andam em um eixo só do grid: **NE** = `linha−1`, **NO** = `coluna−1`, **SE** = `coluna+1`, **SO** = `linha+1`. No código isso é `(sinal(dy − dx), sinal(dy + dx))`. Nos movimentos que mudam linha e coluna juntas, o personagem não pode "cortar a quina" entre duas paredes.
- **Movimento de centro a centro.** Cada passo interpola do centro de um tile ao centro do vizinho. A duração depende da distância **na tela**, porque andar para L/O percorre o dobro de pixels que andar para N/S.
- **Ordem de desenho (profundidade).** O chão é desenhado primeiro, porque nunca cobre nada. Depois paredes, itens, monstros e o jogador entram numa lista **ordenada por `linha + coluna`** do centro de cada um: o que está mais ao fundo é desenhado antes. Assim uma parede na frente esconde o personagem atrás dela.
- **Tileset isométrico gerado a partir de tiles top-down.** O pack 0x72 é desenhado de cima. A ferramenta `ferramentas/gerar_tileset.cpp` aplica a **transformação inversa da projeção** a cada pixel do losango para descobrir de qual pixel do tile original ele vem. As paredes viram **blocos**: topo em losango e duas faces laterais, com sombreamento diferente (luz vindo da esquerda) para dar volume. O resultado é um único `tilesetIso.png` com 17 tiles de 32x32, como pede o formato do enunciado.
- **Regras no arquivo, não no código.** Caminháveis, perigosos, trocas, saída, vitória, animação e objetos vêm todos do `map.txt`, então o código só interpreta as regras e não conhece nenhum índice de tile fixo. A exceção é o tile do cursor.
- **Sprite animado.** O cavaleiro alterna entre a animação **idle** (parado) e **run** (andando), com 4 quadros cada, e espelha horizontalmente conforme o lado para onde anda na tela. Moedas e monstros também são animados por quadros.
- **Pixel art.** As texturas usam `GL_NEAREST` e escala inteira (2x), para os pixels não ficarem borrados.

## 📌 Observações

- Código do jogo: `GrauB/main.cpp`. Gerador do tileset: `GrauB/ferramentas/gerar_tileset.cpp`. Mapa: `GrauB/map.txt`.
- Os caminhos do mapa e dos sprites são definidos no `CMakeLists.txt`, então o executável pode ser rodado de qualquer diretório.

## 🙌 Créditos

- Assets: [0x72 - 16x16 DungeonTileset II](https://0x72.itch.io/dungeontileset-ii) (licença CC0), em `assets/0x72_DungeonTilesetII_v1.7/`.

## 📚 Referências

- Rocha, E. J. T. S. (2003). *Conceitos teóricos dos jogos isométricos*. In *Forge 16V: Um framework para desenvolvimento de jogos isométricos* (Cap. 4, pp. 51-68). Dissertação de mestrado, UFPE.
- [GLFW Input Guide](https://www.glfw.org/docs/3.3/input_guide.html)
