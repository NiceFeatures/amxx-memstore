# AMX Mod X MemStore

[![Build & Package](https://github.com/iceeedR/amxx-memstore/actions/workflows/build.yml/badge.svg)](https://github.com/iceeedR/amxx-memstore/actions/workflows/build.yml)
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-brightgreen.svg)]()
[![Compatibility](https://img.shields.io/badge/AMXX-1.9%20%7C%201.10%20%7C%20ReAMXX-orange.svg)]()

**MemStore** é um módulo de alta performance para **AMX Mod X (GoldSrc / HLDS)** focado em armazenamento de dados puramente em memória RAM, transitórios entre plugins e entre trocas de mapa.

Diferente do `nVault` (que grava em disco) ou do `localinfo` (que possui limite minúsculo de caracteres no HLDS), o **MemStore** atua como uma camada de *In-Memory Key-Value Store* estilo Redis, garantindo zero I/O de disco, isolamento por namespaces, verificação estrita de tipos e políticas flexíveis de ciclo de vida.

---

## ⚡ Principais Recursos

- **Persistência Transitória em RAM**: Sobrevive à rotação de mapas (`changelevel`) sem tocar no disco rígido.
- **Tipos Suportados**: Inteiros (`cell`), Números decimais (`Float:`), Cadeias de caracteres (`string`) e Vetores (`array[]`).
- **Sistema Nativo de Leaderboards / Ranks (ZSET)**:
  - Ordenação automática por score (`mem_rank_set`, `mem_rank_get_top`, `mem_rank_get_pos`).
  - Suporta ordenação decrescente (`RANK_DESC`) para frags/pontos e crescente (`RANK_ASC`) para speedrun/tempos.
  - Desempate automático (*tie-breaking*) determinístico e seguro.
- **Enumeração de Chaves por Índice**:
  - `mem_get_key_at(namespace, index, dest, maxlen)` e `mem_get_namespace_count(namespace)`.
  - Permite iterar por todas as chaves de um namespace sem depender de handles do Pawn.
- **Isolamento por Namespaces**: Múltiplos plugins podem coexistir sem colisão de chaves (`mem_set_*(namespace, key, value)`).
- **Políticas de Expiração Inteligentes**:
  - `EXP_PERSISTENT`: Mantido na RAM indefinidamente até o processo do servidor ser reiniciado.
  - `EXP_MAP_END`: Limpo automaticamente na próxima troca de mapa.
  - `EXP_MAP_COUNT`: Sobrevive por `N` trocas de mapa consecutivas.
  - `EXP_TTL`: Expira após `N` segundos (baseado em tempo real).
- **Strict Type Checking**: Impede corrupção lógica silenciosa com inspeção de tipos via `mem_get_key_type()`.
- **Proteção contra DoS e Esgotamento de RAM**: Limites configuráveis por namespace e tamanho de arrays.
- **Autocontido**: Inclui os headers essenciais do SDK para compilar diretamente sem dependências externas.
- **Compatibilidade Linux (Ubuntu 18.04 / glibc 2.27 / GCC C++11 ABI)**: Construído para compatibilidade universal com servidores Linux de CS 1.6 / HLDS.

---

## 🛠️ Como Compilar

### No Windows (Visual Studio / CMake)

Execute:
```cmd
build.bat
```
Ou manualmente via terminal:
```cmd
cmake -B build -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
O binário compilado será gerado em: `build/Release/memstore_amxx.dll`.

---

### No Linux (Makefile / GCC)

Instale os pacotes 32-bit:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib make
```
Compile:
```bash
make
```
O binário compilado será gerado em: `build_linux/memstore_amxx_i386.so`.

---

## 📦 Estrutura de Instalação no HLDS

Copie os arquivos para o servidor:
```plaintext
addons/
└── amxmodx/
    ├── modules/
    │   ├── memstore_amxx.dll        (Windows)
    │   └── memstore_amxx_i386.so   (Linux)
    └── scripting/
        ├── include/
        │   └── memstore.inc
        ├── test_memstore.sma        (Suíte completa de testes unitários + benchmarks)
        └── example_top15.sma        (Exemplo prático de Top 15 com Ranks e MOTD)
```

No arquivo `addons/amxmodx/configs/modules.ini`, adicione:
```ini
memstore
```

---

## 💡 Exemplo de Código Pawn

### Exemplo 1: Leaderboard Top 15 do Mapa em RAM
```pawn
#include <amxmodx>
#include <memstore>

public OnPlayerKilled(victim, killer)
{
    new killerName[32];
    get_user_name(killer, killerName, charsmax(killerName));

    new currentKills = 0;
    mem_rank_get_score("map_kills", killerName, currentKills);
    
    // Atualiza o ranking nativamente em RAM (EXP_MAP_END limpa automaticamente no novo mapa)
    mem_rank_set("map_kills", killerName, currentKills + 1, EXP_MAP_END);
}

public ShowTopLeader(id)
{
    new leaderName[32], topKills = 0;
    if (mem_rank_get_top("map_kills", 1, leaderName, charsmax(leaderName), topKills, RANK_DESC))
    {
        client_print(id, print_chat, "[MemStore] 1º Lugar do Mapa: %s com %d kills!", leaderName, topKills);
    }
}
```

---

## 🚀 GitHub Actions CI/CD

O repositório já inclui `.github/workflows/build.yml` configurado usando container `ubuntu:18.04` (padrão compatível com servidores Linux HLDS) e `windows-latest` gerando os artefatos `linux32` e `win32` organizados na árvore de diretórios oficial do AMXX.

---

## 📄 Licença

Autor: **iceeedR**
Distribuído sob a licença [GPL v3](https://www.gnu.org/licenses/gpl-3.0.html).
