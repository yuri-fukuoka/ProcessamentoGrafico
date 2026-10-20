# Processamento Gráfico

**Integrante:** Yuri Fukuoka de Oliveira

Trabalhos da disciplina de Processamento Gráfico, em C++ com OpenGL 3.3 (GLFW, GLAD e GLM). Cada pasta tem seu próprio README com controles e explicação.

| Pasta | Trabalho | Alvo do CMake |
|-------|----------|---------------|
| [`DesafioM2-Triangulos`](DesafioM2-Triangulos/) | Triângulos com VAO/VBO e matriz de modelo | `M2_Parte1`, `M2_Parte2` |
| [`DesafioM3-Jogo das Cores`](DesafioM3-Jogo%20das%20Cores/) | Jogo de remover retângulos de cores parecidas | `JogoDasCores` |
| [`DesafioM4-Mapeamento de Texturas`](DesafioM4-Mapeamento%20de%20Texturas/) | Fundo texturizado com adesivos, transformações e filtros | `M4_Texturas` |
| [`DesafioM5-Navegacao em Camadas`](DesafioM5-Navegacao%20em%20Camadas/) | Tappy Plane com parallax em camadas | `M5_Camadas` |
| [`DesafioM6-Jogo em Tilemaps`](DesafioM6-Jogo%20em%20Tilemaps/) | Dungeon Escape em tilemap | `M6_Dungeon` |
| [`Vivencial1`](Vivencial1/) | Triângulos criados a partir de cliques do mouse | `Vivencial1` |
| [`Vivencial2`](Vivencial2/) | Parallax scrolling com foguete controlado pelo teclado | `Vivencial2` |
| [`GrauB`](GrauB/) | Dungeon Escape Isométrico (tilemap diamond, 8 direções) | `GrauB` (e `GrauB_GerarTileset`) |

## Como compilar

Requisitos: CMake 3.16+ e um compilador C++17. GLFW e GLM são baixados automaticamente pelo CMake (`FetchContent`), e o GLAD está em `common/` e `include/`.

```bash
cmake -S . -B build
cmake --build build                      # compila tudo
cmake --build build --target M6_Dungeon  # ou só um trabalho
./build/M6_Dungeon
```

Os caminhos dos assets (`assets/`) são definidos no CMake, então os executáveis podem ser rodados de qualquer diretório.
