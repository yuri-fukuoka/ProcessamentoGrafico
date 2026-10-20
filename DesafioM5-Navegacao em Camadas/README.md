# Desafio M5 - Navegação em Camadas

Cena de jogo 2D em OpenGL 3.3 no estilo *Flappy Bird*: um avião voa por um cenário com **parallax** de várias camadas, desviando de rochas, coletando estrelas e colidindo com o relevo do chão. Assets: [Kenney - Tappy Plane](https://kenney.nl/assets/tappy-plane) (CC0).

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target M5_Camadas
./build/M5_Camadas
```

As texturas são lidas de `assets/kenney_tappy-plane/PNG/` (o caminho é definido no CMake, então o executável pode ser rodado de qualquer diretório).

## Controles

| Tecla / mouse                                 | Ação                                         |
|-----------------------------------------------|----------------------------------------------|
| `ESPAÇO` / `↑` / `W` / clique esquerdo        | impulso do avião (também começa e reinicia)  |
| `P`                                           | pausa / continua                             |
| `H`                                           | mostra / esconde as caixas de colisão        |
| `C`                                           | troca a cor do avião (vermelho, azul, verde, amarelo) |
| `R`                                           | reinicia                                     |
| `ESC`                                         | sai                                          |

**Pontuação:** +1 por par de rochas ultrapassado e +1 por estrela coletada. Na tela de *Game Over* o número grande é a pontuação da partida e o pequeno é o recorde. Medalhas: bronze (5+), prata (15+), ouro (30+). O título da janela também mostra pontos e recorde.

## Camadas (de trás para frente)

| # | Camada              | Textura             | Velocidade (fator) | Observação                                     |
|---|---------------------|---------------------|--------------------|------------------------------------------------|
| 0 | Céu e nuvens        | `background.png`    | 0.10               | quase parado                                   |
| 1 | Montanhas distantes | `groundRock.png`    | 0.30               | 65% de névoa (cor misturada com o céu)         |
| 2 | Colinas             | `groundDirt.png`    | 0.55               | 35% de névoa                                   |
| 3 | Jogo                | rochas, estrelas, fumaça, avião | 1.00   | onde acontecem as interações                   |
| 4 | Chão                | `groundGrass.png`   | 1.00               | cobre a base das rochas; o avião colide com ele |
| 5 | Nuvens frontais     | `puffLarge.png`     | 1.60               | semitransparentes, passam na frente do avião   |

Quanto mais ao fundo, mais devagar a camada rola e mais "apagada" (névoa) ela fica, o que dá a sensação de profundidade.

## Objetos animados e interação com o cenário

- **Avião:** animação por frames (hélice, 3 sprites em sequência 1-2-3-2), gravidade e impulso, rotação conforme a velocidade vertical. Ao bater, cai girando até parar no chão.
- **Rochas:** rolam com a camada do jogo e são recicladas com nova altura aleatória quando saem da tela. Colisão **círculo x triângulo** (o triângulo segue o formato da rocha medido no PNG).
- **Chão:** o perfil de altura de cada coluna é extraído do **canal alfa** da textura, então a colisão acompanha o relevo desenhado (morros e vales).
- **Estrelas:** flutuam e balançam; ao serem tocadas são coletadas com uma animação (crescem, giram, sobem e somem).
- **Fumaça:** partículas emitidas pela traseira do avião que crescem, ficam transparentes e rolam com o cenário.

Pressione `H` durante o jogo para ver os volumes de colisão (círculo do avião, triângulos das rochas, estrelas e linha do relevo do chão).

## Como funciona

- **Geometria:** um único quadrado unitário centralizado na origem; cada objeto usa uma matriz de modelo (translação * rotação * escala). Projeção ortográfica em pixels (800x480), com y para baixo.
- **Parallax:** cada camada é um retângulo da largura da tela com textura em `GL_REPEAT` na horizontal. O fragment shader desloca a coordenada de textura (`uv * repeticoes + deslocamento`), com `deslocamento = scroll * fator / larguraDaTextura`.
- **Névoa:** o shader mistura a cor da textura com a cor do céu (`mix(cor, ceu, nevoa)`).
- **Transparência:** blending alfa e `discard` dos fragmentos totalmente transparentes.
