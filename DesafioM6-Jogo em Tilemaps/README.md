# Desafio M6 - Jogo em Tilemaps: Dungeon Escape

Jogo 2D em OpenGL 3.3 com visão de cima (top-down). O cenário é um **tilemap** lido de um arquivo texto e desenhado com o tileset [Kenney - Tiny Dungeon](https://kenney.nl/assets/tiny-dungeon) (CC0, tiles de 16x16 desenhados na tela em 3x).

O cavaleiro está preso numa masmorra. Para escapar, ele precisa **abrir o baú que guarda a chave** e depois **sair pela porta**, desviando de monstros e espinhos, antes que o tempo acabe.

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target M6_Dungeon
./build/M6_Dungeon
```

O tileset é lido de `assets/kenney_tinyDungeon/Tilemap/tilemap_packed.png` e o mapa de `DesafioM6-Jogo em Tilemaps/mapa.txt`. Os dois caminhos são definidos no CMake, então o executável pode ser rodado de qualquer diretório.

## Controles

| Tecla             | Ação                                          |
|-------------------|-----------------------------------------------|
| `WASD` / setas    | move o cavaleiro (de tile em tile)            |
| `ENTER` / `ESPAÇO`| começa (e recomeça na tela final)             |
| `P`               | pausa / continua                              |
| `R`               | reinicia a fase                               |
| `G`               | mostra / esconde a grade do tilemap           |
| `ESC`             | sai                                           |

## Objetivo, vitória e derrota

- **Objetivo:** encostar no baú (sala de cima, à direita) para pegar a chave. A porta da sala de cima, à esquerda, se abre com uma animação.
- **Vitória:** entrar pela porta aberta.
- **Derrota:** perder as 3 vidas ou deixar o tempo (3:00) acabar.
- Encostar num monstro ou em espinhos levantados custa 1 vida. O jogador volta ao checkpoint e fica alguns instantes invulnerável (piscando). O checkpoint é o início da fase e passa a ser a frente do baú depois que a chave é pega.
- As poções vermelhas devolvem 1 vida (só são pegas se faltar alguma).

O HUD mostra as vidas (cavaleiros), se você já tem a chave e o tempo restante, que fica vermelho e piscando abaixo de 30 s.

## Elementos que mudam de posição

| Elemento      | Movimento                                                                 |
|---------------|---------------------------------------------------------------------------|
| Rato          | circuito retangular na sala inicial                                       |
| Morcegos (2)  | voam em circuito na sala da porta, batendo as asas e oscilando            |
| Espinhos      | sobem e descem em ondas e piscam em vermelho logo antes de subir          |
| Porta / baú   | a porta abre em 4 quadros e a chave sai flutuando do baú                  |

## O tilemap (`mapa.txt`)

O arquivo tem duas seções. Em `[mapa]` cada caractere é um tile:

| Char | Tile                   | Char | Tile                        |
|------|------------------------|------|-----------------------------|
| `#`  | parede                 | `.`  | chão                        |
| `T`  | face de parede com tocha | `G` | face de parede com grade  |
| `E`  | porta de saída         | `^`  | espinhos                    |
| `P`  | início do jogador      | `C`  | baú com a chave             |
| `p`  | poção (+1 vida)        |      |                             |
| `+ x t s a b` | cruz, lápide, mesa, banquinho, bigorna, barril (bloqueiam a passagem) | | |

Em `[inimigos]` cada linha é `tipo modo velocidade x y [x y ...]`. Os tipos são `rato` e `morcego`. Os modos são `vaivem` (vai e volta pelos pontos) e `circuito` (repete os pontos em loop). Dá para mudar a fase editando só esse arquivo, sem recompilar.

## Como funciona

- **Autotile das paredes:** o código decide o desenho de cada `#` pelos vizinhos.
  - Se há chão logo abaixo, a parede é uma **face** de tijolos. As bordas laterais mudam se há chão à esquerda ou à direita.
  - Caso contrário, é o **topo** da parede. O topo é montado em **4 quadrantes de 8x8**, cada um escolhido pelos dois vizinhos ortogonais e pelo diagonal: quina convexa, borda, canto côncavo ou topo liso. Assim qualquer formato de parede fica com a borda de pedra correta.
- **Sombras do chão:** o chão recebe sombra quando há parede acima ou à esquerda. A sombra lateral reaproveita o tile da sombra de cima, só que **transposto** no vertex shader (flags de espelhar e transpor a coordenada de textura).
- **Camadas de desenho:** a camada estática (chão, paredes e decoração) é pré-calculada. Por cima vêm a porta, os espinhos, o baú e as poções, depois os inimigos de chão, o jogador e os voadores. HUD e telas ficam no topo.
- **Movimento:** o jogador anda de tile em tile, com interpolação e um pequeno pulo, e só entra em tiles livres. Os inimigos seguem listas de pontos (vai e volta ou circuito). As colisões são por distância entre centros. Os espinhos só machucam se estiverem levantados no tile em que o jogador está.
- **Iluminação:** o fragment shader escurece a cena (luz ambiente) e soma algumas luzes pontuais: uma em volta do jogador e outras nos itens (o baú pulsa para ajudar a achar e as poções brilham em vermelho). A posição é arredondada para o "pixel" do tileset para manter o visual pixel art.
- **Texto:** o tileset não tem fonte, então os textos usam uma fonte bitmap 5x7 definida no código e gerada como textura na inicialização. O ícone da chave também é um glifo dessa fonte.
- **Geometria:** um único quadrado unitário com projeção ortográfica em pixels (y para baixo) e texturas com filtro `GL_NEAREST`.
