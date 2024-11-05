# WorldWeatherWatcher
Utilisation du Script auto.sh
Prérequis

    Installer screen :
    Si screen n'est pas déjà installé sur votre système, vous pouvez l'installer en utilisant la commande suivante :

    sudo pacman -S screen

Configuration du Script

    Rendre le script exécutable :
    Avant de pouvoir exécuter le script, vous devez le rendre exécutable. Utilisez la commande suivante :

chmod +x ./auto.sh

Modifier le script pour votre environnement :
Vous devrez spécifier le chemin du dossier contenant votre code Arduino dans le script. Ouvrez auto.sh et modifiez la variable FOLDER pour qu'elle pointe vers votre dossier de code :

    FOLDER="le chemin de ton code"

    Remplacez "le chemin de ton code" par le chemin réel où se trouve votre code.

Lancer le Script

    Exécuter le script :
    Une fois que vous avez configuré le script, vous pouvez l'exécuter avec la commande suivante :

./auto.sh f=<nom_du_dossier>

Remplacez <nom_du_dossier> par le nom du dossier contenant votre code.

Utiliser screen pour afficher les messages série :
Pour afficher les messages série de votre Arduino, utilisez screen. Par exemple, pour se connecter à un appareil sur /dev/ttyACM0 avec une vitesse de communication de 115200 baud, utilisez la commande suivante :

    screen /dev/ttyACM0 115200

    Assurez-vous de remplacer /dev/ttyACM0 par le port correct utilisé par votre Arduino.

Pour quitter screen, appuyez sur Ctrl+A, puis K, et confirmez avec Y.
Notes

    Assurez-vous que votre appareil Arduino est connecté et que vous avez les permissions nécessaires pour accéder au port série.
    Les messages série seront affichés dans la session screen.




#main.ino

Téléchargez l'arduino IDE, installez les bibliothèques suivantes : 
    
      Adafruit_BME280_Library
    
      Adafruit_BusIO
    
      Adafruit_DMA_neopixel_library
    
      Adafruit_NeoPixel
    
      Adafruit_Unified_Sensor
    
      RTClib
    
      SD
    
      VEGA_ChainableLED

