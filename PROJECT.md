# activities-overview — Project Brief

> GNOME Activities-style overview for MATE and the Marco window manager: press Super to zoom out all windows, switch workspaces, and type to search apps. Lightweight C/GTK3, X11, no Clutter.

Este documento descreve a ideia, o escopo e as decisões técnicas do projeto. Deve ser lido por completo antes de qualquer implementação.

---

## 1. Motivação

O MATE é muito mais leve que o GNOME, mas não tem nada equivalente à tela **Activities** do GNOME Shell (aberta pela tecla Super). Essa tela junta numa ação só:

1. Visão geral de todas as janelas abertas, com zoom-out animado
2. Navegação e troca entre workspaces
3. Busca de aplicações digitando direto, sem clicar em nada

Essa função é o único motivo para usar GNOME em vez de MATE. O objetivo é reproduzir esse comportamento **o mais fiel possível** no MATE/Marco, sem o peso do GNOME Shell.

Alternativas avaliadas:
- **xfdashboard**: a mais próxima, mas depende de Clutter (descontinuado) + OpenGL, carrega libs do Xfce (xfconf) e é pesada no startup.
- **Compiz Scale/Expo**: obriga a trocar o Marco pelo Compiz.
- **skippy-xd / rofi**: cobrem só uma parte (exposé ou launcher), sem a experiência integrada.

## 2. Objetivo

Um daemon leve que, ao toque da tecla Super, abre um overview fullscreen idêntico em comportamento ao GNOME Activities, rodando sobre Marco (ou qualquer WM EWMH no X11 com compositor).

### Metas de desempenho
| Métrica | Alvo |
|---|---|
| Tempo para abrir (daemon já rodando) | < 100 ms até o 1º frame |
| Animação | 60 fps em GPU Intel integrada |
| RAM ociosa | < 30 MB |
| CPU ociosa | ~0% (sem polling; apenas eventos X/D-Bus) |

## 3. Comportamento esperado (referência: GNOME 3.3x / 40+)

### Abrir / fechar
- **Toque** na tecla Super (pressionar e soltar sem outra tecla no meio) faz toggle do overview.
- `Super + outra tecla` **não** deve disparar o overview (preservar os atalhos existentes).
- `Esc`: se houver texto na busca, limpa; senão, fecha o overview.
- Clique em área vazia fecha.
- CLI: `activities-overview --toggle | --show | --hide`.

### Visão de janelas
- Ao abrir, cada janela do workspace atual **anima da posição/tamanho real até a posição no grid** (~250 ms, ease-out-cubic). Ao fechar, faz a animação inversa.
- Fundo: wallpaper atual escurecido e/ou desfocado.
- Miniaturas **ao vivo** (conteúdo real da janela, atualizado via XDamage).
- Hover destaca a janela, mostra o título abaixo e um botão de fechar (X).
- Clique ativa a janela e fecha o overview com animação.
- Clique do meio fecha a janela.
- Setas/Tab navegam entre janelas; Enter ativa.
- Janelas minimizadas aparecem também (miniatura em cache ou ícone do app).

### Workspaces
- Faixa vertical à direita com miniaturas de cada workspace (wallpaper + janelas em escala).
- Clique em uma miniatura troca para aquele workspace (o overview continua aberto e mostra as janelas dele).
- Arrastar uma janela do grid para uma miniatura move a janela para aquele workspace.
- Scroll do mouse sobre a faixa troca de workspace.
- `Ctrl+Alt+↑/↓` (ou PageUp/PageDown) troca de workspace dentro do overview.

### Busca
- Campo de busca no topo, **focado automaticamente**: basta começar a digitar.
- Ao digitar, a visão de janelas dá lugar aos resultados:
  - **Aplicações** (arquivos .desktop), com ranking igual ao do GNOME (`g_desktop_app_info_search`)
  - **Janelas abertas** cujo título/app combinam com o texto
- Enter abre o primeiro resultado; setas navegam.
- Lançar um app fecha o overview.

### Multi-monitor
- O overview aparece em todos os monitores; cada um mostra as janelas que estão nele.
- Busca e faixa de workspaces ficam no monitor primário.

## 4. Fora de escopo (por ora)
- Wayland (o MATE é X11 na prática)
- Busca de arquivos, calculadora e outros providers de busca do GNOME
- Hot corner (pode ser feito externamente; avaliar depois)
- Dash de favoritos e grade "Mostrar aplicativos" (fase 2)

## 5. Arquitetura

### Stack
| Componente | Escolha | Motivo |
|---|---|---|
| Linguagem | C11 | Padrão do ecossistema MATE e o mais leve |
| UI / loop | GTK3 (`GtkApplication`) | Instância única via D-Bus de graça, CSS para tema |
| Janelas/workspaces | libwnck-3 | Abstração EWMH madura (lista, ativar, mover, fechar) |
| Miniaturas | XComposite + XDamage + XRender | Pixmap real da janela, escala acelerada via XRender |
| Renderização | cairo (`cairo_xlib_surface` sobre os pixmaps) | Sem Clutter/OpenGL direto |
| Animação | `gtk_widget_add_tick_callback` (frame clock) | Sincronizado com vblank |
| Tecla Super | XInput2 **raw events** no root window | Detecta o toque sem grab, sem quebrar os atalhos `Super+X` |
| Busca de apps | GIO `GDesktopAppInfo` + `GAppInfoMonitor` | Ranking nativo, atualiza ao instalar apps |
| Config | GSettings | Padrão MATE, editável via dconf |
| Build | Meson + Ninja | |

### Modelo de execução
- `activities-overview --daemon` sobe via autostart (`/etc/xdg/autostart/activities-overview.desktop`), pré-carrega o estado e fica ocioso.
- As janelas do overview (uma por monitor, fullscreen, ARGB, override-redirect, sempre por cima) são **criadas uma vez e reaproveitadas**: só mapeia/desmapeia.
- Ao abrir: snapshot da geometria das janelas pelo wnck → pixmaps via `XCompositeNameWindowPixmap` → calcula layout → anima.
- Enquanto aberto: assina XDamage só das janelas visíveis. Ao fechar: libera os damages e os pixmaps.

### Compositor
- Miniaturas ao vivo exigem que as janelas estejam redirecionadas (compositor do Marco ligado, ou picom).
- Se não houver compositor: detectar (`_NET_WM_CM_S<n>`) e cair para ícone do app + título, com aviso no log. Nunca forçar redirect por conta própria.

### Layout do grid
- Inspirado no `UnalignedLayoutStrategy` do GNOME: escolher o número de linhas que maximiza a escala média, manter a ordem relativa pela posição original (x, y) para a animação ser natural, nunca ampliar janela além de 100%.
- Implementar como função pura (entrada: retângulos + área disponível; saída: retângulos) para ser testável.

### Estrutura do repositório
```
activities-overview/
├── meson.build
├── meson_options.txt
├── data/
│   ├── activities-overview.desktop          # autostart
│   ├── io.github.guilhermepachecod.ActivitiesOverview.gschema.xml
│   └── style.css                            # tema padrão (GResource)
├── src/
│   ├── main.c              # GtkApplication, CLI, ciclo do daemon
│   ├── overview.c/.h       # janela(s) do overview, show/hide, estados
│   ├── window-grid.c/.h    # grid de janelas, hover, clique, DnD
│   ├── layout.c/.h         # algoritmo de layout (puro, testável)
│   ├── thumbnail.c/.h      # XComposite/XDamage/XRender → cairo
│   ├── workspaces.c/.h     # faixa de workspaces
│   ├── search.c/.h         # entrada, providers (apps, janelas), resultados
│   ├── superkey.c/.h       # detecção do toque em Super via XI2 raw
│   ├── animation.c/.h      # interpolação/easing, tick callback
│   └── background.c/.h     # wallpaper (gsettings org.mate.background)
├── tests/
│   └── test-layout.c
├── debian/                 # empacotamento .deb
├── packaging/rpm/activities-overview.spec
├── .github/workflows/
│   ├── ci.yml              # build + testes em Debian 12, Debian 13, AlmaLinux 9
│   └── release.yml         # em tag v*: gera .deb e .rpm e anexa ao release
├── LICENSE                 # GPL-2.0-or-later
└── README.md
```

### Dependências (Debian)
```
build-essential meson ninja-build pkg-config
libglib2.0-dev libgtk-3-dev libwnck-3-dev
libx11-dev libxcomposite-dev libxdamage-dev libxrender-dev libxi-dev libxfixes-dev
```
AlmaLinux 9: equivalentes `-devel` via EPEL (MATE vem do EPEL).

## 6. Roadmap

| Marco | Entrega |
|---|---|
| **M0** | Scaffold: meson, estrutura, CI compilando nas 3 distros, licença, README |
| **M1** | Daemon + `--toggle` via D-Bus + janela fullscreen com fundo escurecido + Esc fecha |
| **M2** | Grid de janelas com miniaturas estáticas (pixmap), layout, clique ativa |
| **M3** | Animação de abrir/fechar a partir da geometria real; miniaturas ao vivo (XDamage) |
| **M4** | Busca: foco automático, apps + janelas, teclado completo |
| **M5** | Faixa de workspaces: miniaturas, troca, scroll, arrastar janela |
| **M6** | Toque em Super via XI2 (com detecção de conflito com atalhos do MATE) |
| **M7** | Multi-monitor |
| **M8** | Empacotamento: .deb, .rpm, release automatizado no GitHub |
| **M9** | Polimento: GSettings (duração da animação, escurecimento etc.), CSS/temas, fallback sem compositor, traduções (gettext, pt_BR primeiro) |

Cada marco deve terminar compilando sem warnings e utilizável (mesmo que incompleto).

## 7. Convenções

- Código, comentários, commits, issues e README **em inglês**. Este brief está em pt-BR só para alinhamento interno.
- Estilo GLib/GNOME C: `snake_case`, prefixo `ao_` nas funções públicas, GObject apenas onde fizer sentido (widgets), structs simples no resto.
- Compilar com `-Wall -Wextra -Werror` no CI; `-fsanitize=address,undefined` em build de debug.
- Nenhuma dependência nova sem justificar.
- Nada de polling: tudo por eventos (wnck signals, XDamage, D-Bus).
- Liberar sempre pixmaps/damages ao fechar o overview (testar vazamento com valgrind/ASan).
- Commits pequenos, mensagem no padrão Conventional Commits (`feat:`, `fix:`, `build:`...).

## 8. Critérios de aceite do MVP (M1–M6)

- [ ] Toque em Super abre/fecha; `Super+D`, `Super+E` etc. continuam funcionando
- [ ] Janelas animam suavemente até o grid e voltam ao fechar
- [ ] Clique em janela ativa; X fecha; clique do meio fecha
- [ ] Digitar logo após abrir busca apps; Enter abre o primeiro
- [ ] Faixa de workspaces troca e aceita arrastar janelas
- [ ] Abre em < 100 ms com o daemon rodando, num notebook com Intel integrada
- [ ] Funciona no Debian 12/13 com MATE + Marco (compositor ligado)

## 9. Testes manuais de referência

Ambiente principal: Debian 12/13 com MATE e o compositor do Marco ligado
(`gsettings set org.mate.Marco.general compositing-manager true`).
Comparar lado a lado com GNOME (Debian) em outro notebook para fidelidade de comportamento e timing.