# Système d'Authentification

Projet pédagogique en C illustrant la mise en place d'un système d'authentification (inscription / connexion) avec hachage et salage des mots de passe, ainsi qu'un outil d'attaque par force brute permettant d'évaluer la robustesse du système face à ce type de menace.

> ⚠️ **Projet à but éducatif uniquement.** La fonction de hachage utilisée (`simple_hash`) est une fonction de type DJB2, non cryptographique. Elle est volontairement simple afin d'illustrer les failles d'un système d'authentification mal conçu et de comprendre l'intérêt des fonctions de hachage cryptographiques robustes (bcrypt, Argon2, etc.) en conditions réelles.

## Structure du dépôt

```
.
├── auth.c          # Système d'authentification (inscription / connexion)
└── attaque.c       # Outil de brute force contre le système d'authentification
```

Le fichier `password.txt` (base des identifiants) est créé automatiquement à la première inscription. Il n'est pas versionné, pas plus que les exécutables compilés.

## Fonctionnalités

### `auth.c` — Système d'authentification

- **Inscription (`signup`)** : création d'un compte avec validation stricte des identifiants
  - Username : exactement 5 lettres minuscules
  - Password : exactement 8 caractères, avec au moins 1 minuscule, 1 majuscule et 1 chiffre
- **Connexion (`signin`)** : vérification des identifiants par comparaison de hash
- **Salage des mots de passe** : génération d'un sel aléatoire de 5 caractères par utilisateur
- **Protection contre le brute force** : verrouillage progressif du compte après plusieurs échecs
  - 3 tentatives → verrouillage 5 secondes
  - 6 tentatives → verrouillage 10 secondes
  - 7 tentatives → verrouillage 15 secondes
  - 8 tentatives → verrouillage 20 secondes
  - 9 tentatives → bannissement définitif du compte
- **Nettoyage du fichier** (`clean_password_file`) : suppression des lignes mal formées dans `password.txt`

### `attaque.c` — Outil de brute force

Outil permettant de tester la résistance du système d'authentification, avec trois modes :

1. **Mode 1 — Credentials inconnus** : recherche exhaustive du couple username/password (démonstration de la combinatoire massive : 62⁵ usernames × 66⁸ passwords)
2. **Mode 2 — Username connu** : brute force ciblé sur le mot de passe d'un compte identifié
3. **Mode 3 — Dictionnaire puis brute force** : test d'une liste de mots de passe courants avant de basculer en force brute exhaustive

## Format de stockage (`password.txt`)

Chaque ligne suit le format :

```
username:hash:salt
```

Exemple :
```
alice:3491177128:69110
bobby:2851944493:32410
carol:1147841512:75773
```

Le hash est calculé ainsi : `hash = simple_hash(password + salt)`.

## Compilation

```bash
gcc auth.c -o auth
gcc attaque.c -o attaque
```

## Utilisation

**Créer un compte et se connecter :**
```bash
./auth
```

**Tester la robustesse d'un compte (usage pédagogique, sur ses propres comptes de test) :**
```bash
./attaque
```

## Objectifs pédagogiques

Ce projet permet d'illustrer concrètement :
- La différence entre un hachage simple et un hachage cryptographique sécurisé
- L'importance du salage pour contrer les attaques par dictionnaire/rainbow tables
- L'efficacité des mécanismes de verrouillage progressif et de bannissement contre le brute force
- L'ordre de grandeur du temps nécessaire à une attaque exhaustive selon l'espace de recherche (charset et longueur)

## Limites connues

- La fonction de hachage n'est pas cryptographique (collisions possibles, absence de résistance aux attaques par force brute matérielle)
- Le sel est purement numérique (5 chiffres), ce qui réduit l'entropie apportée
- Le fichier `password.txt` est stocké en clair (hash + sel), sans chiffrement du fichier lui-même
- Le blocage des comptes est géré en mémoire (non persistant entre les exécutions)

## Auteure

Développé par **Hana Bouikni** — USTHB, Cybersecurity Engineering.
