#ifndef PARTIE_H
#define PARTIE_H

#include "plateau.h"

#define HUMAIN 0
#define ORDINATEUR 1
#define LONGUEUR_NOM 24
#define FICHIER_SAUVEGARDE "saves/partie.txt"

typedef struct {
    int type;                 /* HUMAIN ou ORDINATEUR */
    int niveau;               /* Niveau de l'IA (1 à 6) */
    char nom[LONGUEUR_NOM];
    int avatar;               /* Image affichée dans l'interface */
    long temps_ms;            /* Temps de réflexion cumulé */
} Joueur;

typedef struct {
    Plateau plateau;
    Joueur joueurs[2];        /* [0] joue les Noirs, [1] les Blancs */
    char trait;               /* Joueur qui doit jouer : NOIR ou BLANC */
    Coup dernier_coup;        /* lig = -1 en début de partie */
    int nb_coups;
    int profondeur;           /* Profondeur de recherche des IA (option -prof) */
    int a_passe;              /* 1 si le joueur précédent vient de passer son tour */
} Partie;

void partie_initialiser(Partie *partie, int depart, const Joueur *noir, const Joueur *blanc, int profondeur);

Joueur *joueur_courant(Partie *partie);

/* Joue le coup du joueur au trait (supposé valide), puis donne la main à
   l'adversaire, ou au même joueur si l'adversaire est bloqué (il passe).
   Renvoie le nombre de pions retournés */
int partie_jouer(Partie *partie, Coup coup);

int partie_finie(const Partie *partie);

/* Renvoie 0 si succès, -1 en cas d'erreur */
int sauvegarder_partie(const Partie *partie, const char *fichier);
int charger_partie(Partie *partie, const char *fichier);
int sauvegarde_existe(const char *fichier);

#endif
