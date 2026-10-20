# Atividade Vivencial 2 - Parallax Scrolling

**Integrante:** Yuri Fukuoka de Oliveira

Extensão do Desafio M4 (mapeamento de texturas e transformações): um **foguete animado** voa pelo espaço controlado pelo teclado, e o cenário, montado em **camadas**, se desloca junto com ele. Camadas próximas se deslocam mais (offset maior) e camadas distantes, menos. Isso cria a ilusão de profundidade (**paralaxe de movimento**).

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target Vivencial2
./build/Vivencial2
```

As texturas são lidas de `assets/` (o caminho é definido no CMake, então o executável pode ser rodado de qualquer diretório).

## Controles

| Tecla / mouse      | Ação                                                     |
|--------------------|----------------------------------------------------------|
| `←` `→` / `A` `D`  | move o foguete para os lados                             |
| `↑` `↓` / `W` `S`  | move o foguete para cima e para baixo                    |
| Clique esquerdo    | o foguete voa até o ponto clicado                        |
| `1` `2` `3`        | troca o foguete (vermelho, verde, azul)                  |
| `R`                | volta à posição inicial (todas as camadas alinhadas)     |
| `ESC`              | sai                                                      |

O título da janela mostra a posição da câmera e o deslocamento da camada mais distante (nebulosa) e da mais próxima (meteoros), o que permite comparar os offsets durante o movimento.

## Camadas (de trás para frente)

| # | Camada            | Arquivo                        | Fator | Tipo                        |
|---|-------------------|--------------------------------|-------|-----------------------------|
| 0 | Nebulosa          | `Background.png`               | 0.05  | textura repetida            |
| 1 | Estrelas pequenas | `Stars.png`                    | 0.12  | textura repetida            |
| 2 | Estrelas grandes  | `Big_Stars.png`                | 0.25  | textura repetida            |
| 3 | Gigante gasoso    | `Gas_Giant.png`                | 0.40  | objeto                      |
| 4 | Luas              | `Moon_1..7.png`                | 0.60  | objetos                     |
| 5 | Foguete           | `Missile_N_Flying_000..009.png`| 1.00  | jogador (a câmera o segue)  |
| 6 | Meteoros          | `Meteor_01..10.png`            | 1.40  | objetos na frente do foguete|

## Como funciona

- **Projeção:** ortográfica com a janela do mundo igual à tela (800x600), então 1 unidade = 1 pixel. O eixo y aponta para baixo.
- **Câmera:** segue o foguete com suavização (`camera += (foguete - camera) * k * dt`). No início a câmera está na origem, por isso **todas as camadas começam com deslocamento zero**.
- **Parallax:** o deslocamento de cada camada é `camera * fator`. O fator representa a distância da camada até a câmera: perto de 0 é muito longe (quase não se mexe), 1 é a distância do foguete e maior que 1 está entre o foguete e a câmera (passa na frente e mais rápido). Vale para os dois eixos, então subir e descer também desloca as camadas, cada uma no seu ritmo.
- **Camadas repetidas** (nebulosa e estrelas): um retângulo do tamanho da tela com a textura em `GL_REPEAT`. O deslocamento é aplicado na **coordenada de textura** no fragment shader: `uv = (uv - 0.5) * repeticoes + 0.5 + deslocamento`, com `deslocamento = camera * fator / tamanhoDaTexturaNaTela`. Essas texturas repetem sem emenda na horizontal, então o cenário é infinito para os lados.
- **Camadas de objetos** (planeta, luas e meteoros): cada objeto tem uma posição na sua camada e é desenhado em `posição - camera * fator`. Na horizontal a posição é "envolvida" num período, então os objetos voltam a aparecer depois de um tempo.
- **Limite vertical:** o foguete sobe ou desce no máximo 200 px. Como os fatores são menores que 1 nas camadas de fundo, a borda das imagens nunca aparece. A nebulosa não repete sem emenda na vertical, e esse limite evita que a emenda apareça.
- **Foguete (transformações e animação):**
  - **Animação por frames:** 10 imagens da chama em sequência, mais rápida quando o foguete se move.
  - **Rotação:** o sprite original aponta para cima, então gira +90° para voar para a direita e −90° para a esquerda. Ao subir ou descer o nariz inclina até 20°. Ao trocar de sentido o ângulo é interpolado e o foguete faz a curva passando pelo "nariz para cima".
  - **Escala:** o sprite é reduzido para 22% do tamanho original.
  - Tudo isso fica na matriz de modelo (translação * rotação * escala).
- **Mouse:** o clique é convertido da tela para o mundo (`camera + clique - centroDaTela`) e vira o destino do foguete (GLFW Input Guide: `glfwSetMouseButtonCallback` e `glfwGetCursorPos`).

## Créditos dos assets

- Fundo: *Parallax Space Background Multipack* (cenário "Gas Giant"), usado apenas para fins educacionais.
- Foguete e meteoros: [Craftpix - Free Space Shooter Game Objects](https://craftpix.net/freebies/) (licença em `assets/craftpix-781145-free-space-shooter-game-objects/TXT/license.txt`).
