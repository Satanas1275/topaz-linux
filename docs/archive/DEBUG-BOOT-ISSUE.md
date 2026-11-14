> 📦 **ARCHIVÉ** — instantané de debug du 12 nov. 2026 (tentative GPU n°1, échouée),
> conservé pour l'historique. Voir `STATUS.md` pour l'état actuel.

# Debug Boot Issue - meow-linux-hardware-acceleration1.img

## Problème

L'image `meow-linux-hardware-acceleration1.img` ne boot pas.

## Images disponibles

1. **meow-linux-drm.img** (9 nov, 64M) → ✅ BOOT OK (baseline)
2. **meow-linux-hardware-acceleration1.img** (12 nov 17:22, 64M) → ❌ NO BOOT (avec GPU fixes)
3. **meow-linux-rollback-test.img** (12 nov 17:27, 64M) → ❓ À TESTER (GPU fixes annulés)

## Changements qui ont été appliqués (puis annulés)

### Change 1: pil_gpu_mem shared-dma-pool
```diff
pil_gpu_mem: memory@56615000 {
+   compatible = "shared-dma-pool";
    reg = <0x0 0x56615000 0x0 0x2000>;
    no-map;
};
```

### Change 2: qcom_scm dload-mode
```diff
firmware {
    scm: scm {
        compatible = "qcom,scm-sm6115", "qcom,scm";
        #reset-cells = <1>;
        interconnects = <&system_noc MASTER_CRYPTO_CORE0 RPM_ALWAYS_TAG
                 &bimc SLAVE_EBI_CH0 RPM_ALWAYS_TAG>;
+       qcom,dload-mode = <&tcsr_regs 0x13000>;
    };
};
```

## Tests à faire

### Test 1: Vérifier que le rollback boot
```bash
fastboot boot ~/linux-topaz/meow-linux-rollback-test.img
```

Si ça boot → Le problème vient d'un des 2 changements

### Test 2: Réappliquer SEULEMENT shared-dma-pool

Si rollback OK, tester avec SEULEMENT le `compatible = "shared-dma-pool"` (sans le qcom,dload-mode)

### Test 3: Réappliquer SEULEMENT qcom,dload-mode

Si rollback OK, tester avec SEULEMENT le `qcom,dload-mode` (sans le shared-dma-pool)

## Hypothèses

### Hyp 1: qcom,dload-mode casse le boot
Le `qcom,dload-mode` référence `&tcsr_regs 0x13000`. Si tcsr_regs n'est pas correctement configuré, ça peut crasher le SCM.

**Solution:** Enlever qcom,dload-mode, garder seulement shared-dma-pool

### Hyp 2: shared-dma-pool change l'adressage mémoire
Marquer pil_gpu_mem comme shared-dma-pool peut changer comment le bootloader/TZ gère cette région.

**Solution:** Agrandir la région ou utiliser une autre approche

### Hyp 3: Le DTB est corrompu
Erreur de compilation du DTB.

**Solution:** Vérifier avec `dtc -I dtb -O dts sm6225-xiaomi-tapas.dtb`

## Commandes de debug

### Vérifier le DTB
```bash
cd ~/Documents/PostmarketOS-redmi-note-12-4g/work/kernel/arch/arm64/boot/dts/qcom
dtc -I dtb -O dts sm6225-xiaomi-tapas.dtb > /tmp/dtb-decoded.dts
grep -A5 "pil_gpu_mem\|qcom,dload-mode" /tmp/dtb-decoded.dts
```

### Comparer les DTB
```bash
# DTB qui boot
cp ~/linux-topaz/meow-linux-drm.img /tmp/
# Extraire le DTB...

# DTB qui ne boot pas  
cp ~/linux-topaz/meow-linux-hardware-acceleration1.img /tmp/
# Extraire le DTB et comparer
```

### Logs si accessible via serial/UART
```bash
# Si tu as un accès serial UART:
cat /dev/ttyUSB0
# ou
minicom -D /dev/ttyUSB0
```

## Prochaine étape

1. **Tester meow-linux-rollback-test.img**
   ```bash
   fastboot boot ~/linux-topaz/meow-linux-rollback-test.img
   ```

2. **Si ça boot:** Le problème vient d'un des changements
   - On teste alors change 1 seul
   - Puis change 2 seul

3. **Si ça ne boot toujours pas:** Le problème est ailleurs
   - Vérifier le kernel Image n'a pas changé
   - Vérifier l'initrd
   - Comparer avec meow-linux-drm.img

## Questions

- **Symptômes exacts:** Le téléphone freeze où? Logo? Écran noir? Reboot loop?
- **Logs accessibles:** Tu as accès aux logs (serial UART, ADB, dmesg)?
- **Dernier boot OK:** meow-linux-drm.img bootait correctement?

---

**État actuel:** Changements GPU annulés, image de rollback créée pour test.
