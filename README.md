# ESP32-C6 Printer Test

Ce projet est un exemple d'utilisation d'une imprimante thermique ESC/POS avec un ESP32-C6 sous ESP-IDF.

## Fonctionnalités

- Initialisation de l'imprimante via UART
- Impression de texte simple, gras, souligné, justifié
- Impression de séparateurs, de lignes avec prix aligné à droite
- Impression de bitmaps (logos)
- Découpe du papier (si supportée)
- Fonctions utilitaires pour l'impression ESC/POS

## Structure du projet
. ├── main/ │ ├── CMakeLists.txt │ └── printer_test_main.c // Exemple d'utilisation ├── components/ │ └── escpos/ │ ├── CMakeLists.txt │ ├── escpos.c // Implémentation des commandes ESC/POS │ └── escpos.h // API publique ESC/POS ├── CMakeLists.txt ├── README.md └── sdkconfig

## Utilisation

1. **Connexion**  
   Branche l'imprimante thermique sur les broches UART de l'ESP32-C6 (voir `TX_PIN` et `RX_PIN` dans `printer_test_main.c`).

2. **Compilation et flash**  
   Compile et flashe le projet avec les outils ESP-IDF :
   ```sh
   idf.py build
   idf.py -p PORT flash monitor
   
3. **Résultat**
    L'imprimante doit imprimer différents exemples de texte, séparateurs, logo, etc.

## Dépendances
 - ESP-IDF
 - Imprimante thermique compatible ESC/POS