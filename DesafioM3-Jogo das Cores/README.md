# Desafio M3 - Jogo das Cores

**Integrante:** Yuri Fukuoka de Oliveira

Jogo em OpenGL 3.3: uma grade de 8x10 retângulos com cores aleatórias. Ao clicar em um retângulo, ele e **todos os retângulos de cor parecida** são removidos. O objetivo é limpar a grade com o menor número de tentativas possível.

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target JogoDasCores
./build/JogoDasCores
```

## Controles

| Tecla / mouse   | Ação                          |
|-----------------|-------------------------------|
| Clique esquerdo | seleciona uma cor             |
| `R`             | reinicia o jogo (novas cores) |
| `ESC`           | sai                           |

## Regras e pontuação

- Duas cores são "parecidas" quando a **distância euclidiana no espaço RGB**, normalizada para 0..1 (dividida por √3, a maior distância possível), é no máximo **0.2** (`TOLERANCIA`).
- Cada tentativa vale **1 ponto a menos por retângulo** que a anterior: a 1ª vale 10 pontos por retângulo removido, a 2ª vale 9, e assim por diante, com mínimo de 1. Limpar a grade com poucas tentativas dá mais pontos.
- O título da janela mostra pontos, tentativas e retângulos restantes. Quando a grade fica vazia, aparece a tela de **fim de jogo** com a pontuação final.

## Como funciona

- **Geometria:** um único quadrado unitário (VAO/VBO) desenhado com `GL_TRIANGLE_STRIP`. Cada retângulo usa uma matriz de modelo (translação + escala) e a cor vai por uniform.
- **Projeção:** ortográfica em pixels (800x600, y para baixo), igual às coordenadas do mouse. Por isso o clique vira linha e coluna da grade com uma simples divisão pelo tamanho da célula.
- **Texto da tela final:** fonte bitmap 5x7 definida no código. Cada pixel aceso da letra é desenhado como um quadradinho, reaproveitando o mesmo quadrado unitário.
- **Transparência:** o fundo escuro da tela final é um retângulo preto com alpha 0.75 (`GL_BLEND`).
