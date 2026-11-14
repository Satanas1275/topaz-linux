# Wi-Fi sous Linux sur topaz (SM6225 / tapas)

Date : 2026-11-09. Travail fait sur l'hôte, **aucun test téléphone**, aucune partition
flashée, aucune image de référence modifiée.

---

## 1. Matériel, firmware, transport, pilote

| Élément | Valeur |
| --- | --- |
 | Puce Wi-Fi | Qualcomm **WCN3988** (variante WCN3990, « Integrated Connectivity Subsystem ») |
| Pilote Linux | **`ath10k_snoc`** (`drivers/net/wireless/ath/ath10k/snoc.c`), compatible DT `qcom,wcn3990-wifi` |
| Transport | Le WLAN tourne sur le **DSP modem (MPSS)** : le MPSS démarre avec `modem.mdt`, demande `wlanmdsp.mbn` par **TFTP/QRTR**, puis `ath10k_snoc` parle **QMI** au service WLFW du modem |
| Noyau | **mainline postmarketOS 7.2.9** (`linux-postmarketos-qcom-sm6225-tapas`), pas le noyau vendor 5.15.41 |
| Firmware modem | `qcom/sm6225/XIAOMI/TAPAS/modem.mdt` + `modem.b00…b29` (48 Mio) |
| Firmware WLAN DSP | `qcom/sm6225/XIAOMI/TAPAS/wlanmdsp.mbn` (3,7 Mio) |
| Firmware ath10k | `ath10k/WCN3990/hw1.0/xiaomi-tapas/firmware-5.bin` + `board.bin` (BDF) |
| Userspace | `tqftpserv` (sert `wlanmdsp.mbn`), `rmtfs -P -s` (démarre le remoteproc modem + sert l'EFS depuis les partitions), `iw` / `wpa_supplicant` |
| EFS (NV) modem | partitions UFS `modemst1`, `modemst2`, `fsc`, `fsg` — **obligatoires** : sans elles le modem plante (`fs_device_efs_buffer api failed`) |

### Pourquoi pas le noyau vendor 5.15.41

- `CONFIG_WLAN_VENDOR_ATH` n'est **pas activé** dans `kernel.config` : aucun pilote ath10k.
- L'arbre vendor ne connaît ni `wcn3988`, ni `icnss`, ni `wlanmdsp` (le DT ROM utilise
  `qcom,icnss`, un pilote propriétaire absent de l'arbre).
- Les modules `vendor_dlkm` sont clang+CFI, inutilisables avec le noyau GCC hôte.

### Pourquoi le noyau mainline 7.2.9

`~/Documents/PostmarketOS-redmi-note-12-4g/work/kernel` a **tout déjà intégré**
(`modules.builtin`) :

```
qmi_helpers  rmtfs_mem  smem  wcnss_ctrl  ath10k_core  ath10k_snoc
qcom_q6v5_mss  qcom_q6v5_pas  qcom_wcnss_pil  qcom_glink  qcom_glink_rpm
qcom_glink_smem  qrtr  qrtr-smd
```

Le DTB `sm6225-xiaomi-tapas.dtb` porte déjà le nœud `wifi@c800000`
(`qcom,wcn3990-wifi`, `firmware-name = "xiaomi-tapas"`,
`qcom,ath10k-calibration-variant = "tapas-gl"`, `status = "okay"`).

### Point important : le MPSS n'a pas `auto_boot`

`qcom,sm6115-mpss-pas` est mappé à `sc8180x_mpss_resource` avec
`auto_boot = false` : le remoteproc modem n'est **pas** démarré par le noyau.
`rmtfs -s` (`rproc_init()` + `rproc_start()`) écrit `start` dans
`/sys/class/remoteproc/<rproc>/state` — c'est ce qui amorce la chaîne Wi-Fi.
Le `/init` de l'initrd a un secours manuel équivalent.

---

## 2. Changements effectués

**Aucun fichier existant n'est modifié.** Nouveaux fichiers :

| Fichier | Rôle |
| --- | --- |
| `rd-wifi/init` | le `/init` de l'initrd Wi-Fi (busybox sh) |
| `tools/build-wifi-initrd.sh` | construit `rd-wifi/initrd-wifi.cpio.gz` (télécharge les paquets APK, extrait le firmware) |
| `linux-payload-wifi.bin` | payload MEOW (`MEOXLINUX`) : noyau PMOS + initrd + DTB |
| `meow-linux-wifi.img` | **image candidate** (boot v4, 100 802 560 o) |
| `rd-wifi/initrd-wifi.cpio.gz` | l'initrd (35,7 Mio compressé, 61,7 Mio décompressé) |

Images de référence conservées (non touchées) :

- `meow-linux-boot.img` (md5 `79ecb82d0074876d822c0d5cc7471335`)
- `meow-linux-drm.img`

---

## 3. Commandes de build (hôte)

```bash
# 1. initrd (télécharge tqftpserv/rmtfs/qrtr-libs de postmarketOS et
#    musl/iw/libnl/wpa_supplicant d'Alpine, extrait le firmware PMOS)
cd ~/linux-topaz
bash tools/build-wifi-initrd.sh

# 2. payload MEOW : noyau PMOS + initrd + DTB patché
python3 ~/meow-os/tools/mklinux.py \
  ~/Documents/PostmarketOS-redmi-note-12-4g/work/kernel/arch/arm64/boot/Image \
  ~/linux-topaz/rd-wifi/initrd-wifi.cpio.gz \
  ~/linux-topaz/linux-payload-wifi.bin \
  --dtb ~/linux-topaz/sm6225-xiaomi-tapas-mem.dtb --dtb-pa 0x94000000 \
  --cmdline 'console=tty0 console=ttyGS0 panic=20 pd_ignore_unused clk_ignore_unused' \
  --kernel-pa 0x85000000 --initrd-pa 0x90000000

# 3. image de boot v4 (shim inchangé)
cd ~/meow-os
MKBOOTIMG="python3 ~/mkbootimg_src/mkbootimg.py" \
  bash shim/build.sh ~/linux-topaz/linux-payload-wifi.bin
cp shim/out/shim-boot.img ~/linux-topaz/meow-linux-wifi.img
```

### Disposition mémoire (vérifiée)

| Région | Adresses |
| --- | --- |
| noyau | `0x85000000 – 0x88dc6a00` |
| initrd | `0x90000000 – 0x9220dcd3` |
| DTB | `0x94000000 – 0x94013c64` |
| réserve `rmtfs` pmOS | `0x89b01000 – 0x89e01000` (non croisée) |

Le DTB est celui de pmOS avec `/memory@80000000 reg = <0 0x80000000 0 0xc0000000>` (3 Gio).

---

## 4. Démarrage de l'image candidate

```
fastboot boot /home/satanas/linux-topaz/meow-linux-wifi.img
```

La console est sur le panneau (`console=tty0`) et, si le gadget USB est disponible,
un shell sur `/dev/ttyGS0` (115200) est proposé.

---

## 5. Logs à récupérer (au premier démarrage)

```sh
# la chaîne complète
dmesg | grep -iE 'ath10k|qmi|qrtr|wcn|wlan|remoteproc|mpss|modem|glink|smem|scm|pas|tz'

# le modem a-t-il démarré ?
ls /sys/class/remoteproc/
cat /sys/class/remoteproc/remoteproc*/state
cat /sys/class/remoteproc/remoteproc*/firmware
cat /sys/class/remoteproc/remoteproc*/coredump 2>/dev/null

# tqftpserv a-t-il servi wlanmdsp.mbn ?
cat /tmp/tqftpserv.log
grep -i 'RRQ\|wlanmdsp' /tmp/tqftpserv.log

# rmtfs / EFS
cat /tmp/rmtfs.log

# l'interface est-elle là ?
ls /sys/class/net/
iw dev
iw dev wlan0 scan

# secours : QRTR / QMI
ls /sys/devices/virtual/net/ | grep -i qrtr
cat /sys/class/remoteproc/remoteproc*/subsys*/uevent 2>/dev/null
```

---

## 6. Licences

| Élément | Licence |
| --- | --- |
| Noyau mainline 7.2.9 (pmOS) | GPL-2.0-only |
| Firmware `firmware-xiaomi-tapas` (modem, wlanmdsp, ath10k BDF) | propriétaire (Xiaomi/Qualcomm) |
| `tqftpserv`, `rmtfs`, `qrtr-libs` | BSD-3-Clause |
| `wpa_supplicant` | BSD-3-Clause |
| `iw` | GPL-2.0 |
| `libnl3` | LGPL-2.1 |
| `musl` | MIT |
| APK Alpine (musl, iw, libnl3, eudev-libs, openssl3, dbus, pcsc-lite) | MIT / GPL-2.0 / LGPL-2.1 / BSD |

Aucun mot de passe Wi-Fi dans le dépôt. Le `/etc/wpa_supplicant.conf` de l'initrd ne
contient qu'une interface de contrôle, pas de réseau.

---

## 7. Risques / étapes suivantes

### Premier essai (2026-11-09) : crash modem `fs_device_efs_buffer api failed`

Le modem démarre puis plante :
```
remoteproc remoteproc0: remote processor is now up
qcom_q6v5_pas 6080000.remoteproc: fatal error received: fs_deviec_efs_buffer api failed
remoteproc remoteproc0: crash detected in modem: type fatal error
```
Cause : `rmtfs` servait un répertoire vide (`-o /usr/share/rmtfs`) — le modem n'a pas
pu ouvrir l'EFS (`modemst1`…), et l'API `fs_device_efs_buffer` a échoué.

Correctif (dans `rd-wifi/init` v2) : `rmtfs -P -r -s`. `-P` fait lire l'EFS
directement depuis les partitions du téléphone via `/dev/disk/by-partlabel`.
Comme udev ne tourne pas, l'init construit les liens depuis l'**uevent du noyau**
(`PARTNAME=…`, émis par le parseur GPT) au lieu d'utiliser udev.

Rechargez l'image candidate (`meow-linux-wifi.img` v2) et regardez sur le panneau :
`meow-wifi: partlabels: …` doit lister les partitions UFS, puis
`meow-wifi: rmtfs log:` et `meow-wifi: === dmesg (radio) ===`.

### Autres pistes si ça plante encore

- **L'EFS est trop grosse pour le tampon `-r`** (16 Mio max en écriture, et le mode
  read-only charge la partition entière en RAM) : essayez `rmtfs -P -s` sans `-r`
  (lecture-écriture, comme sur Android).
- **Sauvegarde EFS** : avant toute écriture, quatrechissez `modemst1/2`, `fsc`, `fsg`
  (`adb shell su -c "dd if=/dev/block/by-name/modemst1" > modemst1.img`) — c'est l'IMEI/NV.
- **MAC** : `ath10k` utilise une MAC aléatoire. La MAC stock est dans `persist/wlan_mac.bin`.
- **Association** : `wpa_supplicant` est dans l'initrd ; `wpa_cli scan` / `scan_results`.

---

## 8. `capability req rejected: 119` — analyse et candidate v4 (2026-11-10)

### Ce que veut dire l'erreur 119

`119` n'est **pas** une erreur QMI standard (les codes standard s'arrêtent à
`QMI_ERR_NOT_SUPPORTED_V01` = 94). C'est une erreur **spécifique au service WLFW** :

```c
#define QMI_ERR_PLAT_CCPM_CLK_INIT_FAILED 0x77   /* = 119 */
```

Le pilote vendor `icnss` (kernel/msm, `drivers/soc/qcom/icnss.c`) le traite ainsi :

```c
if (resp.resp.error == QMI_ERR_PLAT_CCPM_CLK_INIT_FAILED)
        icnss_pr_err("RF card Not present");
```

Donc : la PD WLAN du modem (`wlanmdsp.mbn`) n'a **pas réussi à initialiser
l'horloge/led CCPM de la puce RF**. Ce n'est pas un problème de séquence QMI, et
sauter `WLFW_CAP_REQ` ne peut que déplacer l'échec (le BDF download renvoie alors
`QMI_ERR_INCOMPATIBLE_STATE` = 90, car le firmware est dans un état invalide).

### Séquence QMI attendue vs ath10k_snoc

| Étape | icnss (vendor) | cnss2 (Qualcomm) | wlfwd (userspace) | ath10k_snoc |
| --- | --- | --- | --- | --- |
| mise sous tension WLAN | **avant** la séquence | avant (bus) | fait par icnss | **après** `FW_READY` |
| `IND_REGISTER` (0x20) | oui | oui | oui | oui |
| `HOST_CAP` (0x34) | non | oui | non | oui |
| `MSA_INFO` (0x2D) / `MSA_READY` (0x2E) | oui | oui | fait par le noyau | oui |
| `CAP` (0x24) | oui | oui | oui | oui |
| `BDF_DOWNLOAD` (0x25) | oui (en userspace) | oui (`bdf_type`) | oui (`bdf_type`) | oui |
| `CAL_REPORT` (0x26) | oui | oui | oui | oui |

La séquence de `ath10k_snoc` est correcte ; le problème est **l'alimentation**.
`ath10k_snoc` n'appelle `ath10k_hw_power_on()` (régulateurs + horloges) qu'une
fois le `FW_READY` reçu (`ath10k_core_probe_fw()`), c'est-à-dire **après** que le
modem a déjà lancé `wlanmdsp.mbn`. Les rails PMIC sont donc encore éteints au
moment du init CCPM. Sur SDM845, les rails WLAN sont `regulator-always-on` dans
le DTS ; sur tapas ils ne le sont pas.

### Correctif v4 (DT uniquement, opt-in)

`sm6225-xiaomi-tapas-wifi-v4.dtb` (généré par `tools/make-wifi-v4-dtb.py`) :

1. `regulator-always-on` sur `pm6125_l8` (vdd-cx-mx), `l16` (vdd-1.8-xo),
   `l17` (vdd-1.3-rfa), `l23` (vdd-3.3-ch0) — les rails du WLAN restent
   alimentés au boot, avant que `rmtfs -s` ne démarre le modem.
2. Le nœud wifi déclare `clock = <…>` (singulier) ; le framework clock ne parse
   que `clocks` (`drivers/clk/clk.c`, `of_parse_clkspec()`), donc
   `devm_clk_bulk_get_optional()` ne trouvait jamais `cxo_ref_clk_pin` et
   l'horloge XO n'était jamais activée. Renommé en `clocks`.
3. Le quirk `qcom,snoc-capability-skip-quirk` (expérimental, v3) n'est **pas**
   posé ; le noyau pmOS est rebuild sans le patch skip-CAP.

### Fichiers / image

| Fichier | Rôle |
| --- | --- |
| `sm6225-xiaomi-tapas-wifi-v4.dtb` | DTB pmOS + correctifs (généré) |
| `kernel-wifi-Image-v4` | noyau pmOS 7.2.9 **sans** le patch skip-CAP |
| `rd-wifi-v4/initrd-wifi-v4.cpio.gz` | initrd (sans le test WPA v3) |
| `linux-payload-wifi-v4.bin` | payload MEOW |
| `meow-linux-wifi-v4.img` | **image candidate** (md5 `88b3c7c9db8d1068156d8a497ba70e50`) |
| `tools/make-wifi-v4-dtb.py` | régénère le DTB |
| `tools/package-wifi-candidate-v4.sh` | rebuild l'image |

L'image stable `meow-linux-boot.img` (md5 `79ecb82d0074876d822c0d5cc7471335`)
n'est pas touchée.

### Commandes de test

```sh
fastboot boot meow-linux-wifi-v4.img
# sur le panneau / ttyGS0 :
dmesg | grep -iE 'ath10k|qmi|qrtr|wcn|wlan|remoteproc|mpss|modem'
cat /tmp/tqftpserv.log /tmp/rmtfs.log
cat /sys/class/remoteproc/remoteproc*/state
ls /sys/class/net/ ; iw dev ; iw dev wlan0 scan
```

### Logs exacts à récupérer si ça ne suffit pas

- La ligne complète `ath10k_snoc …: capability req rejected: …` (et ce qui suit).
- `dmesg | grep -iE 'regulator|pm6125|vreg'` — vérifier que les rails sont bien
  à 1 / à la tension attendue.
- `dmesg | grep -iE 'clk|rpmcc|rf_clk'` — vérifier que `cxo_ref_clk_pin` est
  activé (ou au moins trouvé).
- `cat /sys/kernel/debug/clk/clk_summary` (si debugfs monté) pour l'état de
  `rf_clk2`.
- `dmesg | grep -iE 'rpm|smd'` — le PMIC passe par le RPM/Smd.
- Si le CCPM échoue toujours : le `<reason>` du PDM (`PDM: service 'wlan_process'
  crash: …`) et le coredump du modem.

---

## 9. Succès validé en direct : Wi-Fi v6 opérationnel (2026-11-11)

### Résumé de l'étape franchie
Le Wi-Fi fonctionne désormais parfaitement sur le **Redmi Note 12 4G (SM6225 / topaz / tapas)** avec le noyau Linux mainline 7.2.9 sans aucun contournement sale :

1. **Modem remoteproc (MPSS) :** Lancé avec succès par `rmtfs -P -r -s -v` (état `running`).
2. **Firmware WLAN DSP :** `tqftpserv` a servi `wlanmdsp.mbn` (3,9 Mio) via QRTR sans erreur.
3. **Partitions EFS :** Lues directement depuis `/dev/disk/by-partlabel` (`modemst1`, `modemst2`, `fsc`, `fsg`).
4. **Séquence QMI ath10k :**
   - Régulateurs PMIC `pm6125` (l8, l16, l17, l23) en `regulator-always-on` dans le DT.
   - Horloge `cxo_ref_clk_pin` correctement reconnue (`clocks = <&rpmcc RPM_SMD_RF_CLK2>`).
   - L'étape QMI `CAP_REQ` passe sans erreur (aucun code 119 `CCPM_CLK_INIT_FAILED`).
   - `ath10k_snoc` monte le firmware WCN3988/WCN3990 (`wlan-fw-2-signed`, `htt-ver 3.56`).
5. **Interface réseau et connectivité :**
   - Création de `wlan0`.
   - Scan des réseaux Wi-Fi fonctionnel via `iw`.
   - `wpa_supplicant` associe le téléphone au réseau **`OuiFi`** (`wpa_state=COMPLETED`).
   - Obtention d'adresse IP par DHCP (`udhcpc`).
   - Daemon SSH **Dropbear** accessible sur le port 22.
   - **Shell root interactif obtenu à distance via SSH (`~ #`) !**

Candidate de référence : `meow-linux-wifi-v6.img`
Commande de démarrage : `fastboot boot meow-linux-wifi-v6.img`
Connexion SSH : `ssh -i ~/.ssh/topaz_ed25519 root@<IP>`
