# INIT_GITHUB — Setup do repositório e fluxo de push

Repositório: https://github.com/guilhermepachecod/activities-overview
Remote (SSH): `git@github.com:guilhermepachecod/activities-overview.git`
Branch principal: `main`

Leia também o `PROJECT.md` (escopo e convenções do projeto).

---

## 1. Setup inicial (executar uma única vez)

O repositório no GitHub foi criado **vazio** (sem README, LICENSE nem .gitignore). Não há nada para fazer pull antes do primeiro push.

```bash
cd ~/activities-overview          # diretório do projeto, onde estão PROJECT.md e INIT_GITHUB.md

git init
git branch -M main
git config user.name  "Guilherme Pacheco"
git config user.email "<email do GitHub>"   # confirmar com o usuário se não estiver configurado globalmente

git remote add origin git@github.com:guilhermepachecod/activities-overview.git
ssh -T git@github.com              # deve responder "Hi guilhermepachecod!"
```

### Arquivos obrigatórios no primeiro commit
1. **`.gitignore`**
   ```gitignore
   # Meson / build
   build/
   builddir/
   _build/
   *.o
   *.so
   *.a
   compile_commands.json

   # Empacotamento
   *.deb
   *.rpm
   *.buildinfo
   *.changes
   debian/.debhelper/
   debian/activities-overview/
   debian/files
   debian/*.substvars
   debian/*.log

   # Editores / SO
   .vscode/
   .idea/
   *.swp
   *~
   .DS_Store

   # Debug
   core
   vgcore.*
   *.log
   ```
2. **`LICENSE`**: texto completo da GPL-2.0 (https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt). O projeto é GPL-2.0-or-later.
3. **`README.md`** (em inglês): nome, a descrição abaixo, status "work in progress", dependências e build com meson.
   > GNOME Activities-style overview for MATE and the Marco window manager: press Super to zoom out all windows, switch workspaces, and type to search apps. Lightweight C/GTK3, X11, no Clutter.
4. `PROJECT.md` e `INIT_GITHUB.md`

```bash
git add .gitignore LICENSE README.md PROJECT.md INIT_GITHUB.md
git commit -m "chore: initial project brief, license and readme"
git push -u origin main
```

### Ajustes no GitHub (feitos pelo usuário ou via `gh`)
```bash
gh repo edit guilhermepachecod/activities-overview \
  --description "GNOME Activities-style overview for MATE and the Marco window manager: press Super to zoom out all windows, switch workspaces, and type to search apps. Lightweight C/GTK3, X11, no Clutter." \
  --add-topic mate,mate-desktop,marco,x11,gtk3,libwnck,activities,overview,workspace-switcher,app-launcher
```

---

## 2. Fluxo padrão: TODA alteração termina em push

Regra: **nenhum trabalho fica só local.** Ao concluir cada unidade lógica de trabalho (uma feature, um fix, um passo de um marco do roadmap), siga os passos abaixo.

```bash
# 1. Sincronizar antes de começar
git pull --rebase origin main

# 2. ... fazer as alterações ...

# 3. Garantir que compila e passa nos testes (quando já existir meson.build)
meson setup build --wipe 2>/dev/null || meson setup build
meson compile -C build
meson test -C build

# 4. Revisar o que vai entrar
git status
git diff

# 5. Commit (Conventional Commits, em inglês)
git add <arquivos específicos>        # evitar "git add ." às cegas
git commit -m "feat(grid): animate windows from real geometry to grid"

# 6. Push
git push origin main
```

### Regras
- **Não fazer push com build quebrado.** Se não compila, não sobe. A exceção é o M0, antes de existir código.
- **Commits pequenos e atômicos**: um assunto por commit.
- **Nunca** commitar `build/`, binários, `.deb`/`.rpm`, credenciais, tokens ou chaves.
- **Nunca** usar `git push --force` na `main`. Se o push for rejeitado, faça `git pull --rebase origin main`, resolva os conflitos e tente de novo.
- Ao fechar um marco do roadmap (M0, M1…), mencionar no commit: `feat: complete M2 window grid (#M2)`.
- Depois do push, conferir o resultado do CI: `gh run list --limit 1` / `gh run watch`.

### Prefixos de commit
| Prefixo | Uso |
|---|---|
| `feat:` | Nova funcionalidade |
| `fix:` | Correção de bug |
| `perf:` | Desempenho |
| `refactor:` | Refatoração sem mudar comportamento |
| `build:` | meson, dependências, empacotamento |
| `ci:` | GitHub Actions |
| `docs:` | README, PROJECT.md, comentários |
| `test:` | Testes |
| `chore:` | Manutenção geral |

Escopo opcional entre parênteses: `grid`, `search`, `workspaces`, `superkey`, `thumbnail`, `layout`, `anim`, `packaging`.

---

## 3. Releases (a partir do M8)

```bash
# atualizar a versão em meson.build (project(... version: 'X.Y.Z'))
git commit -am "build: bump version to 0.1.0"
git tag -a v0.1.0 -m "v0.1.0"
git push origin main --tags
```
A tag `v*` dispara o workflow `release.yml`, que gera o `.deb` e o `.rpm` e anexa ao GitHub Release.

Versionamento: SemVer. Use `0.x` até o MVP (M1–M6) estar completo e `1.0.0` quando os critérios de aceite do `PROJECT.md` forem atingidos.

---

## 4. Branches (opcional, quando o projeto crescer)

Enquanto for um mantenedor só, o trabalho vai direto na `main`. Para mudanças grandes ou experimentais:
```bash
git switch -c feat/workspace-dnd
# ... commits ...
git push -u origin feat/workspace-dnd
gh pr create --fill --base main
```