# État du projet

*Dernière mise à jour : 2026-11-14.* Cible : Redmi Note 12 4G (`topaz` / `tapas`, SM6225).
Démarrage par le shim MEOW (`fastboot boot <image>`), pas de flash permanent.

| Fonction | État | Détails |
| --- | --- | --- |
| Boot Linux | ✅ OK | via le shim MEOW ; image de référence `meow-linux-boot.img` |
| Affichage / console | ✅ OK (DRM/KMS) | noyau mainline postmarketOS 7.2.9 (`meow-linux-drm.img`, `meow-linux-wifi-v6.img`) : `msm-drm` (DPU) + panneau `panel-xiaomi-m7-38-0c-0a`, émulation fbdev → `/dev/fb0` + fbcon sur le panneau, `/dev/dri/card0` pour l'userspace. `meowfb` n'existe plus que sur l'image de référence `meow-linux-boot.img` (noyau vendor 5.15.41, framebuffer du bootloader sans DRM/KMS ni backlight) |
| Wi-Fi | ✅ OK | image `meow-linux-wifi-v6.img` : noyau mainline postmarketOS + `ath10k_snoc`, modem lancé par `rmtfs`, `wlanmdsp.mbn` servi par `tqftpserv` (voir `docs/WIFI.md`). SSH par clé uniquement (dropbear `-s`) |
| Accélération GPU (2D/3D) | ❌ Non fonctionnelle | le pilote Adreno 610 est dans le noyau mais l'accélération ne marche pas ; tentative n°1 (`pil_gpu_mem` en `shared-dma-pool` + `qcom,dload-mode`) : l'image ne boot pas, tout a été rollback. Cause racine non identifiée. Voir `docs/archive/` |
| USB | ❌ Non fonctionnel *(à confirmer)* | variantes d'init TCPC dans `rd/init-tcpc*` ; statut à compléter |
| Phosh / mobile userspace | ⏳ Pas commencé | l'affichage DRM/KMS est là ; rendu logiciel (llvmpipe) possible en attendant le GPU, pas essayé |

## À savoir

- Les images (`*.img`, `*.bin`, `*.cpio.gz`) ne sont normalement pas versionnées (voir `.gitignore`), à part quelques-unes
  poussées avant cette règle. Ne pas en committer de nouvelles : utiliser les *Releases*.
- Le `docs/archive/` contient les instantanés de debug des essais ratés (GPU n°1, rollback), gardés pour l'historique.

## Pour reproduire le Wi-Fi

Voir `docs/WIFI.md` (build de l'initrd, paquets, firmware) et `tools/package-wifi-candidate-v6.sh`.
