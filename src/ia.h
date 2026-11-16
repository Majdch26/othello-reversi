#ifndef IA_H
#define IA_H

#include "plateau.h"

/* Intelligence artificielle : un niveau par étape du sujet.

     1  Hasard           : un coup possible tiré au hasard (étape 1)
     2  Débutant         : arbre de jeu de profondeur 2, minimax, évaluation
                           par le nombre de pions (étape 3)
     3  Intermédiaire    : arbre de profondeur réglable (-prof), minimax,
                           évaluation des groupes de 2 et 3 pions (étape 4)
     4  Confirmé         : élagage alpha-bêta (les branches inutiles ne sont
                           pas créées), évaluation pondérée par région (étape 5)
     5  Avancé           : alpha-bêta sur un plateau unique (jouer / annuler),
                           l'arbre n'est jamais stocké en mémoire (étape 6)
     6  Expert           : approfondissement itératif, tri des coups,
                           évaluation plus fine et fin de partie calculée
                           exactement (étape 7)                               */

#define NIVEAU_MIN 1
#define NIVEAU_MAX 6
#define PROFONDEUR_MAX 10
#define VICTOIRE 100000

/* Nœud de l'arbre de jeu (niveaux 2 à 4). Les fils sont chaînés par
   pointeurs : premier_fils, puis frere pour les suivants. */
typedef struct Noeud {
    Coup coup;                  /* Coup qui mène à ce nœud (lig = -1 : passe) */
    char trait;                 /* Joueur qui doit jouer dans cette position */
    Plateau plateau;            /* Position après le coup */
    int valeur;                 /* Valeur remontée par le minimax */
    struct Noeud *premier_fils;
    struct Noeud *frere;
} Noeud;

typedef struct {
    long noeuds;                /* Positions examinées */
    long noeuds_en_memoire;     /* Nœuds de l'arbre alloués (niveaux 2 à 4) */
    int valeur;                 /* Valeur du coup choisi */
    int profondeur;             /* Profondeur atteinte */
} Statistiques;

/* Fonctions d'évaluation : qualité de la position pour joueur */
int evaluation_pions(const Plateau *p, char joueur);
int evaluation_groupes(const Plateau *p, char joueur);
int evaluation_ponderee(const Plateau *p, char joueur);
int evaluation_expert(const Plateau *p, char joueur);
int evaluation_plateau(const Plateau *p, char joueur, int niveau);

/* Crée l'arbre de jeu complet de la profondeur donnée à partir de la
   position où c'est à trait de jouer */
Noeud *creer_arbre_position(const Plateau *p, char trait, int profondeur);
void liberer_arbre(Noeud *racine);
long taille_arbre(const Noeud *racine);

/* Minimax sur un arbre déjà construit, du point de vue de ia */
int minimax_arbre(Noeud *n, char ia, int niveau);

/* Choisit le coup de l'ordinateur. profondeur <= 0 : profondeur par défaut
   du niveau. stats peut valoir NULL. Si aucun coup n'est possible, renvoie
   un coup de ligne COUP_PASSE_LIG */
Coup coup_ordinateur(const Plateau *p, char joueur, int niveau, int profondeur, Statistiques *stats);

int profondeur_par_defaut(int niveau);
const char *nom_niveau(int niveau);

#endif
