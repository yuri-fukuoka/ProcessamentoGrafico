# Desafio M4 - Mapeamento de Texturas

Aplicação OpenGL 3.3 que desenha uma textura de fundo (terra) em um retângulo do tamanho da janela e permite colar **adesivos** (frutas em PNG com transparência) por cima. Os adesivos podem ser movidos, girados, escalados e espelhados, e cada objeto pode receber um filtro de imagem.

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target M4_Texturas
./build/M4_Texturas
```

As texturas são lidas da pasta `assets/` (o caminho é definido no CMake, então o executável pode ser rodado de qualquer diretório):

- Fundo: `assets/Ground070_1K-JPG/Ground070_1K-JPG_Color.jpg`
- Adesivos: `assets/apple.png`, `assets/banana.png`, `assets/grape.png`, `assets/orange.png`

## Controles

| Tecla / mouse          | Ação                                                    |
|------------------------|---------------------------------------------------------|
| Clique esquerdo        | cola um adesivo novo / seleciona e arrasta um existente |
| Clique direito         | remove o adesivo sob o cursor                           |
| `1` `2` `3` `4`        | escolhe a fruta: maçã, banana, uva, laranja             |
| `TAB`                  | alterna a seleção (fundo -> adesivos)                   |
| `Q` / `E` (segurar)    | gira o adesivo selecionado                              |
| `Z` / `X` ou roda      | diminui / aumenta o adesivo selecionado                 |
| `H` / `V`              | flip horizontal / vertical do objeto selecionado        |
| `F`                    | troca o filtro do objeto selecionado                    |
| `DELETE` / `BACKSPACE` | remove o adesivo selecionado                            |
| `C`                    | remove todos os adesivos                                |
| `ESC`                  | sai                                                     |

O título da janela mostra a fruta do próximo adesivo, o objeto selecionado, o filtro e os flips ativos.

## Como funciona

- **Geometria:** um único quadrado unitário centralizado na origem; cada objeto usa uma matriz de modelo (translação * rotação * escala). A projeção ortográfica trabalha direto em pixels.
- **Transparência:** o fragment shader descarta (`discard`) os fragmentos com alpha < 0.1, então só a fruta aparece sobre o fundo.
- **Flip:** feito no fragment shader invertendo a coordenada de textura (`st.x = 1 - st.x` / `st.y = 1 - st.y`).
- **Filtros:** nenhum, tons de cinza, sépia, negativo, binarização, desfoque (média 9x9), bordas (Sobel) e pixelização.
- **Fundo repetido:** a textura de terra é "tileable" e usa `GL_REPEAT`; os adesivos usam `GL_CLAMP_TO_EDGE`.
