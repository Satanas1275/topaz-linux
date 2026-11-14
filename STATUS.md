# État du projet

*Dernière mise à jour : 2026-11-14.* Cible : Redmi Note 12 4G (`topaz` / `tapas`, SM6225).
Démarrage par le shim MEOW (`fastboot boot <image>`), pas de flash permanent.

| Fonction | État | Détails |
| --- | --- | --- |
| Boot Linux | ✅ OK | via le shim MEOW ; image de référence `meow-linux-boot.img` |
| Console / affichage | ✅ OK (basique) | `meowfb` : framebuffer mappé sur ce que le bootloader a allumé, `/dev/fb0` 1080×2400, fbcon. **Pas** de DRM/KMS, de backlight ni de contrôle du panneau avec ce chemin → inutilisable pour Phosh/wlroots (voir `docs/DIAGNOSTIC.md`) |
| Wi-Fi | ✅ OK | image `meow-linux-wifi-v6.img` : noyau mainline postmarketOS + `ath10k_snoc`, modem lancé par `rmtfs`, `wlanmdsp.mbn` servi par `tqftpserv` (voir `docs/WIFI.md`). SSH par clé uniquement (dropbear `-s`) |
| Accélération GPU (2D/3D) | ❌ Non fonctionnelle | tentative n°1 (`pil_gpu_mem` en `shared-dma-pool` + `qcom,dload-mode`) : l'image ne boot pas, tout a été rollback. Cause racine non identifiée. Voir `docs/archive/` |
| USB | ❌ Non fonctionnel *(à confirmer)* | variantes d'init TCPC dans `rd/init-tcpc*` ; statut à compléter |
| Phosh / mobile userspace | ⏳ Pas commencé | bloqué par le DRM/KMS et le GPU |

## À savoir

- Les images (`*.img`, `*.bin`, `*.cpio.gz`) ne sont normalement pas versionnées (voir `.gitignore`), à part quelques-unes
  poussées avant cette règle. Ne pas en committer de nouvelles : utiliser les *Releases*.
- Le `docs/archive/` contient les instantanés de debug des essais ratés (GPU n°1, rollback), gardés pour l'historique.

## Pour reproduire le Wi-Fi

Voir `docs/WIFI.md` (build de l'initrd, paquets, firmware) et `tools/package-wifi-candidate-v6.sh`.
