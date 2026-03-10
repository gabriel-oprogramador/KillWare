# KillWare

## Em desenvolvimento...
![Windows](https://img.shields.io/badge/Windows-green)
![Linux](https://img.shields.io/badge/Linux-lightgrey)
![Web](https://img.shields.io/badge/Web-lightgrey)

## 🌎 Links Oficiais
[![YouTube](https://img.shields.io/badge/YouTube-Channel-yellow?logo=youtube)](https://www.youtube.com/@gabriel-oprogramador)
[![Itch.io](https://img.shields.io/badge/Itch.io-Page-yellow?logo=itch.io&logoColor=white)](https://gabriel-oprogramador.itch.io/)

## Enredo
**KillWare** é um **FPS Runner** ambientado na **Intra-Net**, uma rede virtual onde consciências se conectam.  
Você é um caçador de **Wares** - Corra, atire, desvie e limpe servidores infectados antes de ser expulso da rede.  

## Projeto
O jogo é desenvolvido em **C puro**, utilizando uma base própria focada em:

- **ECS-True**
- **Data-Oriented Design (DoD)**
- **Estrutura orientada a dados (cache-friendly)**
- **Controle total do pipeline gráfico**

Sem engines ou bibliotecas externas (SDL, SFML, GLFW, GLU ou GLM).  
Renderização feita diretamente com **OpenGL**, escrita do zero.  
Assets carregados via `stb_*`.

## Objetivo
Este projeto serve como:
- Base para estudo de arquitetura ECS real
- Desenvolvimento de jogos 2D/3D low-level
- Exploração de performance e controle de memória

## Windows Build
### Ferramentas Unix-like
Para compilar seu projeto no Windows com ferramentas similares ao Unix, você pode usar o **[W64devkit](https://github.com/skeeto/w64devkit)**.
- Baixe e instale o W64devkit.
- Certifique-se de adicionar o path das ferramentas ao seu ambiente (`PATH`).
- O GCC já vem incluído, pronto para uso.

### Usando Clang ao invés do GCC
Caso prefira usar **Clang**, você tem duas opções principais:
#### 1. Clang com MINGW
- Pré-configurado para Windows, inclui tudo pronto para compilar.
- Baixar: [llvm-mingw-20251216-msvcrt-x86_64.zip](https://github.com/mstorsjo/llvm-mingw/releases/tag/20251216)
- Versão utilizada: **LLVM 21.1.8**
#### 2. Clang com MSVC
- Requer **Visual Studio** com suporte a C++ instalado.
- Baixar: [LLVM-21.1.8-win64.exe](https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.8)

### Resumo das diferenças
- **MSVC:** precisa do Visual Studio, integra com compilador da Microsoft.
- **MINGW / W64devkit:** já vem com GCC/Clang e ferramentas prontas, sem necessidade de Visual Studio.