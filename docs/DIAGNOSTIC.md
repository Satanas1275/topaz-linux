# Diagnostic — affichage Linux sur topaz, avant Phosh

> **Note (2026-11-14)** : les §1–2 décrivent l'affichage de l'image de référence (noyau vendor 5.15.41 + `meowfb`).
> Les images sur noyau mainline pmOS (`meow-linux-drm.img`, `meow-linux-wifi-v*.img`) utilisent `msm-drm` (§6) : voir `STATUS.md`.

Date : 2026-11-09. Travail fait sur l'hôte, **aucun test téléphone**, aucune partition flashée.

---

## 1. L'état actuel : `meowfb`

Le démarrage de référence `fastboot boot meow-linux-boot.img` utilise :

- le shim MEOW OS (`~/meow-os/shim`) ;
- le noyau vendor 5.15.41 (`~/linux-topaz/kernel-src`, MiCode `topaz-t-oss`) ;
- le module `meowfb` (`~/linux-topaz/tools/meowfb/meowfb.c`), chargé par `/init` de l'initramfs.

`meowfb` n'est **pas** un pilote de dalle : il ne fait que mapper la mémoire physique
que le bootloader a déjà allumée, et l'enregistrer comme framebuffer.

| Élément | Valeur |
| --- | --- |
| Device | `/dev/fb0` (framebuffer) + `/dev/meowfb` (misc, mmap direct) |
| Résolution | 1080 × 2400 |
| Stride | 4320 octets (4 octets/pixel) |
| bpp | 32 |
| Format | red@16, green@8, blue@0, transp@24 → `0xAARRGGBB` (a8r8g8b8) |
| Console | `console=tty0`, fbcon bind sur `vtcon1`, police `TER16x32` (67×75) |
| Enregistrement | au chargement du module (aucun nœud DT, aucun probe) |

### Limites pour Phosh

- **Pas de DRM/KMS** : pas de `/dev/dri/card0`, pas de mode setting, pas de vsync,
  pas de page flip, pas de GPU. `meowfb` ne fait que `cfb_fillrect` / `cfb_copyarea` /
  `cfb_imageblit` sur un tampon unique.
- **Pas de backlight** : le panneau reste à la luminosité du bootloader.
- **Pas de contrôle du panneau** : ni allumage, ni extinction, ni réinitialisation.
- **wlroots** (le backend de Phosh) exige un backend **DRM/KMS** (ou headless) ;
  le backend fbdev a disparu de wlroots. Donc `meowfb` **ne peut pas** porter Phosh.

---

## 2. Les trois pistes demandées

### a) Framebuffer du shim (`meowfb`) — OK console, incompatible Phosh

Voir §1. C'est un affichage de secours, pas un sous-système graphique.

### b) `simple-framebuffer` / `simpledrm` — inexploitable sur le noyau vendor

- Le noyau vendor a bien `CONFIG_SYSFB_SIMPLEFB=y` et `CONFIG_FB_SIMPLE=y`.
- **Mais** le driver `simplefb` du noyau vendor ne sonde que les **enfants de `/chosen`** :
  `drivers/video/fbdev/simplefb.c:559-562` (`for_each_child_of_node(of_chosen, np)`).
- Le shim injecte son nœud `framebuffer@5c000000` comme enfant de la **racine**
  (`shim/fdt.c` : `fdt_add_framebuffer`, inséré à `root_end`). Donc `simplefb` ne le
  voit jamais → `meowfb` garde `fb0`. C'est cohérent avec le fait que l'affichage
  actuel soit porté par `meowfb`.
- Même avec le nœud sous `/chosen` : `simpledrm` donnerait un device DRM **sans**
  pilote de dalle, sans backlight, sans GPU. Phosh tournerait en rendu logiciel,
  sans aucun contrôle du panneau. Le noyau vendor n'a de toute façon aucun pilote
  de dalle pour ce panneau.

### c) Portage DRM/KMS Qualcomm + pilote de dalle topaz — la seule voie réelle

Le noyau vendor **ne peut pas** être utilisé :

- `drivers/gpu/drm/msm/msm_drv.c` : la table `dt_match` contient `qcom,mdp4`,
  `qcom,mdss`, `qcom,sdm845-mdss`, `qcom,sc7180/7280`, `qcom,sm8150/8250`…
  **mais pas `qcom,sde-kms`**, qui est le compatible du nœud d'affichage du DTB
  ROM/dtbo. Le driver ne binderait jamais.
- Aucun pilote de dalle Xiaomi dans l'arbre vendor :
  `CONFIG_DRM_PANEL_XIAOMI_M7_38_0C_0A` n'existe pas.
- Le module vendor `msm_drm.ko` (50 Mo, extrait de `vendor_dlkm.img`) référence
  `__cfi_slowpath_diag` / `__ubsan_handle_cfi_check_fail_abort` (clang + CFI) ;
  le noyau compilé avec le GCC hôte ne les exporte pas → chargement impossible.

La voie qui marche est le **noyau mainline postmarketOS 7.2.9 déjà construit sur
cet hôte** + son DTB mainline + son paquet firmware :

- `~/Documents/PostmarketOS-redmi-note-12-4g/work/kernel/arch/arm64/boot/Image` (64 Mio)
- `~/Documents/PostmarketOS-redmi-note-12-4g/dist/sm6225-xiaomi-tapas.dtb`
- `~/Documents/PostmarketOS-redmi-note-12-4g/firmware/firmware-xiaomi-tapas-1.tar.xz`

Config du noyau pmOS : `CONFIG_DRM_MSM=y` (DPU, DSI, PHY 7 nm),
`CONFIG_DRM_PANEL_XIAOMI_M7_38_0C_0A=y`, `CONFIG_DRM_MSM_KMS_FBDEV=y`,
`CONFIG_DRM_FBDEV_EMULATION=y`, `CONFIG_FRAMEBUFFER_CONSOLE=y`,
`CONFIG_DEVMEM=y`, GPU Adreno 610 (`adreno-y`, intégré).

DTB pmOS : `display-subsystem@5e00000` (`qcom,sm6225-mdss`) → `qcom,sm6225-dpu` ;
`dsi@5e94000` (`qcom,sm6115-dsi-ctrl`) ; PHY `qcom,sm6375-dsi-phy-7nm` ;
`panel@0` (`xiaomi,m7-38-0c-0a`) ; `gpu@5900000` (`qcom,adreno-610.0` + GMU).

---

## 3. Deux blocages concrets du passage au noyau pmOS par le shim

1. **Nœud `/memory` vide.** Le DTB pmOS porte
   `memory@80000000 reg = <0x0 0x80000000 0x0 0x0>` (taille 0). ABL le remplit
   lorsqu'il boote le `boot.img` pmOS directement, mais le shim **remplace** le DTB
   d'ABL par celui du payload (`shim/linux.c` : `if (h->dtb_pa && h->dtb_size) { dtb = h->dtb_pa; lcopy(...) }`).
   Le noyau ne voyait donc **aucune RAM**. C'est le blocage de la tentative
   précédente : `linux-payload-pmos-usb.bin` embarque le même DTB non patché
   (vérifié : `fdtget ... /memory@80000000 reg` → `0 80000000 0 0`).

2. **Chevauchement de la réserve `rmtfs`.** Le DTB pmOS réserve
   `memory@89b01000` (`compatible = "qcom,rmtfs-mem"`, `no-map`, `0x300000`),
   soit `0x89b01000-0x89e01000`. L'adresse `kernel_pa` par défaut de `mklinux.py`
   (`0x88000000`) + l'image (`0x3fa0000`) = `0x8bfa0000` → **chevauche** la réserve
   `no-map`. Il faut une adresse au-dessus du payload (`0x83f41ee4`) et sous
   `0x89b01000` → **`0x85000000`** (image `0x85000000-0x88fa0000`).

---

## 4. Changements effectués (le plus petit, réversible)

**Aucun changement** du shim, du noyau, de l'initrd de référence, ni de
`meow-linux-boot.img`.

1. DTB pmOS patché hors ligne : `/memory@80000000 reg = <0x0 0x80000000 0x0 0xc0000000>`
   (3 Gio au-dessus de `0x80000000`, couvre kernel / initrd / DTB).
   Fichier : `sm6225-xiaomi-tapas-mem.dtb` (l'original n'est pas modifié).
2. Nouvel initrd `rd/init-drm` → `initrd-drm.cpio.gz` : busybox + un `/init` qui
   ne charge **pas** les modules vendor 5.15.41, rapporte l'état DRM
   (`/sys/class/drm`, `/dev/dri/card0`, `fb0`, panel, backlight, dmesg filtré),
   met en place la console USB `ttyGS0` si un UDC est présent, et ne sort jamais.
3. Payload `linux-payload-drm.bin` (`mklinux.py`) avec
   `kernel_pa=0x85000000`, `initrd_pa=0x90000000`, `dtb_pa=0x94000000`.
4. Image candidate `meow-linux-drm.img` (boot v4, 66 240 512 octets), construite
   avec le shim **inchangé**.

---

## 5. Vérifications (hôte uniquement)

- [x] `meow-linux-drm.img` : boot image **v4**, `kernel_size = 66 232 036`.
- [x] Payload `MEOXLINUX` présent à l'offset `0x38000`
      (page 1 + `align4K(_end)` = `0x37000`).
- [x] Noyau embarqué **identique octet à octet** à
      `~/Documents/PostmarketOS-redmi-note-12-4g/work/kernel/arch/arm64/boot/Image`.
- [x] Initrd embarqué = `init` + `bin/busybox`.
- [x] DTB embarqué = pmOS avec le nœud `/memory` patché
      (`fdtget -t x … /memory@80000000 reg` → `0 80000000 0 c0000000`),
      nœuds `qcom,sm6225-mdss`, `qcom,sm6225-dpu`, `qcom,sm6115-dsi-ctrl`,
      `qcom,sm6375-dsi-phy-7nm`, `xiaomi,m7-38-0c-0a` présents.
- [x] Aucun chevauchement mémoire :
      - kernel `0x85000000-0x88fa0000`
      - initrd `0x90000000-0x9011841b`
      - DTB    `0x94000000-0x94013c64`
      - réserves pmOS : `rmtfs 0x89b01000-0x89e01000`, `splash 0x5c000000-0x5cf00000`,
        `removed 0x60000000-0x63900000`, etc. → aucun croisement.
- [x] `meow-linux-boot.img` **inchangé** (md5 `79ecb82d0074876d822c0d5cc7471335`).
- [ ] **Non vérifié** : le démarrage réel du noyau pmOS sur le téléphone
      (la consigne interdit tout test téléphone). Le candidate prouve
      l'emballage, pas le fonctionnement.

---

## 6. Pilote qui prend le contrôle de l'écran

- **`msm-drm`** (`CONFIG_DRM_MSM`, chemin DPU) + **`panel-xiaomi-m7-38-0c-0a`**
  (`CONFIG_DRM_PANEL_XIAOMI_M7_38_0C_0A`) + PHY DSI 7 nm + GPU Adreno 610
  (composant `adreno`).
- Émulation fbdev → `/dev/fb0`, fbcon → la console apparaît sur le panneau.
- `/dev/dri/card0` pour les clients userspace.

## 7. Compatibilité réelle avec Phosh / wlroots

- **wlroots** a besoin d'un device DRM/KMS : `msm-drm` le fournit. C'est le
  prérequis qui manquait avec `meowfb`.
- **Rendu** : EGL/GBM via mesa `freedreno` (Adreno 610) si le composant GPU
  binder ; sinon `llvmpipe` (rendu logiciel, lent mais fonctionnel).
- **Backlight** : le pilote de dalle déclare un backlight `BACKLIGHT_RAW`
  (`panel-xiaomi-m7-38-0c-0a.c:291`) → Phosh peut régler la luminosité.
- **Phosh n'a pas été démarré** : le candidate n'a pas été booté. Aucune
  affirmation sur le bon fonctionnement de Phosh n'est faite ici.

---

## 8. Commande à lancer

```
fastboot boot /home/satanas/linux-topaz/meow-linux-drm.img
```

Image de référence à conserver (ne pas écraser) :

```
/home/satanas/linux-topaz/meow-linux-boot.img
```
