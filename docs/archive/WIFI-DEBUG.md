> 📦 **ARCHIVÉ** — instantané de debug du 12 nov. 2026 (tentative GPU n°1, échouée),
> conservé pour l'historique. Voir `STATUS.md` pour l'état actuel.

# Debug Wi-Fi Issue - meow-linux-wifi-v6.img

## Problème rapporté

meow-linux-wifi-v6.img n'arrive plus à se connecter au Wi-Fi

## Questions importantes

1. **Qu'as-tu fait exactement?**
   - As-tu flashé `meow-linux-hardware-acceleration1.img`?
   - Ou seulement testé avec `fastboot boot`?

2. **État actuel du téléphone:**
   - Quelle image est flashée en permanence?
   - Tu testes avec `fastboot boot` ou `fastboot flash`?

3. **Symptômes exacts:**
   - L'interface wlan0 existe? (`ip link`)
   - Le Wi-Fi scan fonctionne? (`iw dev wlan0 scan`)
   - wpa_supplicant démarre? (`ps aux | grep wpa`)
   - Erreurs dans dmesg? (`dmesg | grep -i wifi`)

## Hypothèses

### Hyp 1: Tu as flashé mon image qui ne boot pas
Si tu as fait `fastboot flash boot meow-linux-hardware-acceleration1.img`, 
le bootloader a peut-être été corrompu ou une partition modifiée.

**Solution:** Reflash l'image d'origine qui fonctionnait

### Hyp 2: wifi-v6.img n'a pas changé MAIS le téléphone est dans un état bizarre
Après avoir essayé de booter sur mon image cassée, le téléphone est peut-être
dans un état qui empêche le Wi-Fi de fonctionner.

**Solution:** Reboot complet, ou reflash vendor_boot/vbmeta

### Hyp 3: J'ai modifié quelque chose dans linux-topaz sans le savoir
Mais c'est peu probable car je n'ai touché qu'au kernel PostmarketOS.

**Solution:** Vérifier l'historique git

## Tests à faire

### Test 1: Vérifier que wifi-v6.img n'a pas été modifiée
```bash
cd ~/linux-topaz
ls -lh meow-linux-wifi-v6.img
stat meow-linux-wifi-v6.img | grep Modify
# Date devrait être: 11 nov 22:25
```

### Test 2: Vérifier l'état git de linux-topaz
```bash
cd ~/linux-topaz
git status
git diff
# Ne devrait montrer AUCUN changement
```

### Test 3: Reflash complet
```bash
# Revenir à une image stock ou une vieille image qui marchait
fastboot flash boot <ancienne-image-qui-marchait>.img
fastboot reboot
```

### Test 4: Tester avec une image encore plus vieille
```bash
# Tester avec meow-linux-drm.img ou autre
fastboot boot ~/linux-topaz/meow-linux-drm.img
```

## Commandes de diagnostic

```bash
# Sur le téléphone (si accessible)
ip link show
iw dev
dmesg | grep -i "wifi\|ath10k\|wlan"
ps aux | grep wpa
```

## Ce que je SAIS que je n'ai PAS touché

✅ Je n'ai PAS modifié linux-topaz/kernel-src
✅ Je n'ai PAS modifié linux-topaz/rd-wifi-v6/
✅ Je n'ai PAS recompilé le kernel 5.15.41
✅ Je n'ai PAS modifié meow-linux-wifi-v6.img

## Ce que j'AI modifié (mais dans un autre kernel)

❌ PostmarketOS kernel 7.2.9 (~/Documents/PostmarketOS-redmi-note-12-4g/)
❌ Device Tree sm6225.dtsi (mais rollback fait)
❌ Créé de nouvelles images avec le kernel PostmarketOS

## Prochaine étape

**DIS-MOI EXACTEMENT:**
1. Qu'as-tu flashé? (flash ou boot?)
2. Quelle image est actuellement sur le téléphone?
3. Comment tu testes wifi-v6.img? (boot ou flash?)
4. Quels sont les symptômes exacts?

Avec ces infos je pourrai t'aider efficacement!
