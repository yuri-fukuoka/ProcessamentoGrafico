# Desafio M2 - Triângulos

**Integrante:** Yuri Fukuoka de Oliveira

Dois programas em OpenGL 3.3 que introduzem a criação de geometria na GPU (VBO/VAO), shaders e a matriz de transformação.

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target M2_Parte1 M2_Parte2
./build/M2_Parte1
./build/M2_Parte2
```

## Parte 1 - Vários triângulos sem matriz (`Parte1.cpp`)

- **Exercício 1:** a função `createTriangle(x0, y0, x1, y1, x2, y2)` recebe os 3 vértices em coordenadas normalizadas (NDC, de -1 a 1), cria o VBO com os dados, configura o VAO e devolve o VAO.
- **Exercício 2:** cria 5 triângulos com essa função, guarda os VAOs em um `std::vector` e, no loop de renderização, percorre o vetor desenhando cada um com uma cor diferente (uniform `fc`).
- Como não há matriz, o vertex shader usa a posição direto: `gl_Position = vec4(vp, 1.0)`.

| Tecla | Ação |
|-------|------|
| `ESC` | sai  |

## Parte 2 - Triângulos no clique com matriz de modelo (`Parte2.cpp`)

- **Exercício 3:** existe **um único VAO** com o triângulo padrão `v0 (-0.1, -0.1)`, `v1 (0.1, -0.1)`, `v2 (0.0, 0.1)`. Cada triângulo da cena é só uma `struct Triangle { posição, cor }`.
- A cada clique, a posição do mouse é convertida de pixels (y para baixo) para NDC (y para cima) e um novo triângulo é criado ali, com cor RGB sorteada.
- Ao desenhar, a **matriz de modelo** (`glm::translate`) leva o triângulo padrão para a posição de cada instância. Assim a mesma geometria é reaproveitada, mudando só a transformação e a cor.

| Tecla / mouse   | Ação                                   |
|-----------------|----------------------------------------|
| Clique esquerdo | cria um triângulo na posição clicada   |
| `C`             | limpa a tela                           |
| `ESC`           | sai                                    |

O título da janela mostra quantos triângulos existem.
