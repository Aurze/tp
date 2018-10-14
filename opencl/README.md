# log645-tp4
OpenCL - Remise: Mercredi 3 août 2016 avant 23h59 (3 périodes)

[Énoncé](https://cours.etsmtl.ca/log645/private/Documents/Laboratoires/Labo_04.pdf)

Site du serveur: [log645-srv-1.logti.etsmtl.ca](log645-srv-1.logti.etsmtl.ca)

FTP du serveur: [sftp://log645-srv-1.logti.etsmtl.ca](sftp://log645-srv-1.logti.etsmtl.ca)

# Paramètres
Votre programme recevra donc les paramètres suivants dans cet ordre :
* m : Le nombre de colonnes de la matrice de travail (peut varier de 3 à 10000)
* n : Le nombre de lignes de la matrice de travail (peut varier de 3 à 10000)
* np : Le nombre d’itérations à effectuer (peut varier de 1 à 10000)
* td : Taille d’un pas de temps discrétisé
* h : Taille d’une subdivision

_Vous pouvez débugger votre programme avec des paramètres. Pour ce faire, ouvrez les propriétés du projet, allez sous « Configuration properties -> Debugging » et écrivez votre paramètre dans l’entrée « Command Arguments »._

# Contraintes
Dans le cas des deux problèmes, le logiciel doit effectuer le traitement séquentiellement, puis à l’aide de OpenCL. Le logiciel doit aussi afficher le temps d’exécution et la matrice finale dans les deux cas ainsi que l’accélération de la version parallèle vis-à-vis de la version séquentielle.
* Contrairement au laboratoire précédent, _il n’y a pas de usleep. Comme l’application roule sur Windows, il n’y a pas non plus de script de lancement, ni de makefile_. Tout doit simplement rouler dans Visual Studio 2010.
* Votre programme ne doit provoquer aucun avertissement de compilation.

# Objectifs
Ce laboratoire vise à explorer les fonctionnalités de la librairie OpenCL, à déterminer l’accélération et
l’efficacité d’un programme utilisant la programmation sur GPGPU (General-Purpose processing on
Graphics Processing Units) et à comparer le fonctionnement avec les librairies OpenMP et MPI utilisées
durant les laboratoires précédents.

Le problème traité est exactement le même que celui du laboratoire #3. Il s’agit de simuler le transfert
de chaleur sur une plaque en deux dimensions. La simulation doit se faire à l’aide de la librairie OpenCL
afin d’exploiter le traitement sur la carte graphique.
Énoncé du problème

On partitionne une plaque en subdivisions (m par n). La température de la plaque évolue dans le temps
pour un certain nombre de pas de temps (np). La taille du pas de temps (td) et la taille d’un côté d’une
subdivision (h) influence le transfert de chaleur entre les subdivisions, mais n’affecte pas votre
implémentation. Contrairement au laboratoire précédent, **il n’y a pas de paramètre pour le nombre de
processus puisque OpenCL utilise plutôt un nombre d’unités de calcul concurrentes selon le matériel**.

# Environnement de travail
Environnement de travail
Pour ce laboratoire, vous utiliserez **Visual Studio _2010_** sur **Windows**. La procédure pour configurer
OpenCL dans Visual Studio est en annexe. Cette procédure ne s’applique qu’aux cartes NVidia et elle
requiert l’installation du SDK disponible ici : https://developer.nvidia.com/cuda-downloads .
Vous trouverez aussi en annexe, une fonction pour lire un fichier .cl, un squelette de kernel (le contenu
du fichier .cl) et la liste des fonctions OpenCL à utiliser pour rouler ledit kernel.

# Livrables
* La **solution Visual Studio 2010 (.sln)** et ses dépendances dans la structure de dossier originale (je dois pouvoir décompresser le dossier, ouvrir la solution et compiler/rouler votre programme sans étape supplémentaire)
* Le fichier de projet (.vcxproj)
* Le ou les fichiers C++ contenant votre code (.cpp)
* Le fichier OpenCL contenant votre kernel (.cl)
* Ne m’envoyez PAS l’exécutable, les .obj, .pdb, .log, .user, .filters, .sdf, .ipch ou autres fichiers qui n’iraient pas dans un source control. Soit ce sont des fichiers résultants (donc redondants avec le code source), soit ils contiennent des informations sur votre machine (et sur l’utilisateur) que vous ne voudriez probablement pas partager.
* Un rapport au format PDF
* Compressez tous les fichiers pour n’avoir qu’un seul fichier d’archive

# Rapport
### Introduction
### Temps d’exécution
### Résultats (5%)
### Discussion (5%)
### **Questions (20%)**
* Quelle est l’accélération de votre version parallèle relativement à votre version séquentielle?
* Comment est-ce que les unités d’exécution communiquent-elles entre elles?
* Comment la communication se compare-t-elle avec MPI?
* Comment la communication se compare-t-elle avec OpenMP?
* Pourquoi est-il nécessaire d’utiliser la fonction clCreateBuffer plutôt que de simplement passer un pointeur vers les données à la carte vidéo?
### Conception de l’algorithme
* Code (50%)
* Conception (10%)
* Discussion (10%)

### Conclusion
