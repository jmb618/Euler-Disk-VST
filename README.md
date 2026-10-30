# Euler Disk v7 — plugin VST3 (JUCE + Faust)

Trois disques d'Euler (Darmendrail & Muller) en synthese Faust, interface pixel
minimaliste : un disque 3D fil de fer par voix, qui precesse a la vitesse Omega
calculee par le DSP.

## Compiler (Windows, depuis "x64 Native Tools Command Prompt for VS 2022")
    cd D:\dev\EulerDisk
    build.bat
Resultat : build\EulerDisk_artefacts\Release\VST3\Euler Disk.vst3
           build\EulerDisk_artefacts\Release\Standalone\Euler Disk.exe
Copier le dossier .vst3 dans C:\Program Files\Common Files\VST3\

## Interface
- En haut : LANCER (maintenu, une note MIDI fait pareil), POSITION, barre de progression.
- 3 disques ; sous chacun : Diametre, Vitesse, Pan, Niveau de sa voix.
- Glisser verticalement sur un disque = diametre. Double clic = valeur par defaut.
- Reglages globaux groupes : PIECE / METAL / SORTIE.
- En bas : saturation (ON/OFF, Drive, Chaleur) + tube pixel qui s'allume avec le drive.
- RALENTI : facteur d'affichage (1/1, 1/5, 1/10, 1/20, 1/50).

## Vitesse de rotation affichee
Chaque disque tourne a EXACTEMENT Omega / RALENTI tours par seconde (Omega = bargraph
du DSP, en Hz). Omega va de ~3 Hz (gros disque au debut) a plusieurs centaines de Hz :
un ecran 60 i/s ne peut pas l'afficher en temps reel, d'ou le facteur RALENTI, identique
pour les 3 disques. 1/1 = vitesse reelle (lisible tant que Omega reste sous ~10 Hz).

## Fichiers
    dsp/euler_disk.dsp   ton v7 + 4 bargraphs/voix (viz_*) : le son est inchange
    dsp/faust_arch.cpp   architecture Faust minimale
    Source/              plugin JUCE (parametres generes depuis le .dsp)

## Correctifs de stabilite (v7.1)
- **Son qui s'arretait jusqu'au redemarrage** : le limiteur utilisait `ba.slidingMean`
  (integrateur) dont l'erreur d'arrondi s'accumule sans fin ; le gain finissait a 0.
  Remplace par une somme directe (sans memoire infinie) + min-hold : plafond garanti a 0,95.
- Compteur de progression en double precision (`-double`) : plus de blocage a mi-piece
  avec de longues durees / frequences d'echantillonnage elevees.
- Watchdog : tout NaN / Inf detecte remet automatiquement le DSP a zero.
- `prepareToPlay` ne reinitialise plus le DSP si la frequence ne change pas
  (la piece en cours n'est plus interrompue par l'hote).
- Impulsion LANCER tenue 2 blocs (un clic court n'est plus perdu) ; LANCER n'est plus sauvegarde.
