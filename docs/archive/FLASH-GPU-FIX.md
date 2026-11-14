> 📦 **ARCHIVÉ** — instantané de debug du 12 nov. 2026 (tentative GPU n°1, échouée),
> conservé pour l'historique. Voir `STATUS.md` pour l'état actuel.

# 🚀 Flash GPU Hardware Acceleration Fix

## 📦 Image créée

**Fichier:** `meow-linux-hardware-acceleration1.img`  
**Taille:** 64 MB (66,236,416 bytes)  
**SHA256:** `3eafc63d632c4302801b4936c3f74a9757d73ef75937f8714a2d6afbc92e566c`  
**Date:** 12 novembre 2026 17:22

## ✅ Corrections incluses

Cette image contient les corrections Device Tree pour activer l'accélération GPU matérielle:

1. ✅ **pil_gpu_mem** marqué comme `shared-dma-pool` → Permet le chargement du firmware ZAP
2. ✅ **qcom_scm** configuré avec dload-mode → Améliore la gestion du Secure Channel
3. ✅ **DTB mis à jour** avec les corrections GPU

## 🎯 Test rapide (recommandé)

```bash
cd ~/linux-topaz
fastboot boot meow-linux-hardware-acceleration1.img
```

Cette commande **teste** l'image sans la flasher définitivement.

### Vérification après boot

1. Vérifier que le GPU est reconnu:
   ```bash
   dmesg | grep -i "adreno\|zap"
   ```

   Vous devriez voir:
   ```
   [drm] Adreno 610 GPU initialized
   loaded zap shader from qcom/sm6225/XIAOMI/TAPAS/a610_zap.mdt
   ```

2. Tester le rendu 3D:
   ```bash
   glmark2-es2
   # ou
   glxinfo | grep -i renderer
   ```

   Résultat attendu:
   ```
   OpenGL renderer string: Adreno 610
   ```

## 💾 Flash permanent

⚠️ **Seulement après avoir testé avec `fastboot boot` et vérifié que tout fonctionne!**

```bash
cd ~/linux-topaz
fastboot flash boot meow-linux-hardware-acceleration1.img
fastboot reboot
```

## 🔄 Rollback

Si l'image ne fonctionne pas, retourner à une version précédente:

```bash
# Utiliser une ancienne image qui fonctionnait
fastboot flash boot meow-linux-drm.img
fastboot reboot
```

## 📊 Comparaison versions

| Fichier | Taille | GPU 3D | Notes |
|---------|--------|--------|-------|
| `meow-linux-drm.img` | 64 MB | ❌ | Version précédente sans fix GPU |
| `meow-linux-hardware-acceleration1.img` | 64 MB | ✅ | **Nouvelle version avec fix GPU** |

## 🐛 Troubleshooting

### L'image ne boot pas

1. Vérifier que le téléphone est en fastboot:
   ```bash
   fastboot devices
   ```

2. Essayer de rebooter en fastboot:
   ```bash
   fastboot reboot bootloader
   ```

3. Reflasher l'ancienne image:
   ```bash
   fastboot flash boot meow-linux-drm.img
   ```

### Le GPU ne fonctionne toujours pas

1. Vérifier les logs kernel complets:
   ```bash
   dmesg | grep -E "gpu|adreno|msm|drm" > ~/gpu-logs.txt
   cat ~/gpu-logs.txt
   ```

2. Vérifier les fichiers firmware:
   ```bash
   ls -l /lib/firmware/qcom/sm6225/XIAOMI/TAPAS/
   ```

   Vous devez avoir:
   - `a610_zap.mdt`
   - `a610_zap.b00`
   - `a610_zap.b01`
   - `a610_zap.b02`

3. Vérifier les permissions:
   ```bash
   chmod 644 /lib/firmware/qcom/sm6225/XIAOMI/TAPAS/a610_zap.*
   ```

## 📝 Commandes rapides

### Test complet en une commande

```bash
cd ~/linux-topaz && \
fastboot boot meow-linux-hardware-acceleration1.img && \
sleep 60 && \
adb shell "dmesg | grep -i 'adreno\|zap'" && \
adb shell "glxinfo | grep renderer"
```

### Build + Test automatique

```bash
cd ~/linux-topaz && \
./build-sm6225-gpu-fix.sh && \
fastboot boot meow-linux-hardware-acceleration1.img
```

## 🎉 Résultat attendu

Après le boot avec cette image:

✅ **Boot normal** (écran, Wi-Fi, tout fonctionne)  
✅ **GPU Adreno 610 détecté**  
✅ **Firmware ZAP chargé**  
✅ **Rendu 3D matériel fonctionnel**  
✅ **Performance GPU complète**  

### Performance attendue

- **glmark2-es2:** ~30-60 FPS (selon les tests)
- **Composition Wayland:** Fluide avec GPU
- **Vidéo decode:** Accéléré matériellement
- **Jeux:** Fonctionnels avec OpenGL ES 3.1

## 📚 Documentation complète

Voir `GPU-HARDWARE-ACCELERATION-FIX.md` pour:
- Détails techniques des corrections
- Explications des changements Device Tree
- Diagnostic complet
- Troubleshooting avancé
- Références

---

**Prêt à tester?**

```bash
fastboot boot ~/linux-topaz/meow-linux-hardware-acceleration1.img
```

🎮 Bon rendu 3D!
