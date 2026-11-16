#ifndef PLATEAU_H
#define PLATEAU_H

/* Règles du Reversi / Othello, sans aucune dépendance graphique.
   Les lignes et les colonnes sont numérotées de 0 à 7 : la case (0, 0) est
   A1 en haut à gauche, la case (7, 7) est H8. */

#define TAILLE 8
#define NB_CASES (TAILLE * TAILLE)
#define PIONS_PAR_JOUEUR 32

#define VIDE  '.'
#define NOIR  'N'
#define BLANC 'B'

/* Départ du jeu :
   - DEPART_REVERSI : plateau vide, les 4 premiers pions sont posés à tour de
     rôle sur d4, d5, e4, e5, au choix des joueurs (règle du sujet) ;
   - DEPART_OTHELLO : les 4 pions centraux sont déjà placés en croix. */
#define DEPART_REVERSI 0
#define DEPART_OTHELLO 1

typedef struct {
    int lig;
    int col;
} Coup;

/* Coup spécial "passer son tour" */
#define COUP_PASSE_LIG (-1)

typedef struct {
    char cases[TAILLE][TAILLE];
    int depart;               /* DEPART_REVERSI ou DEPART_OTHELLO */
    int stock[2];             /* Pions restants dans la réserve : [0] noir, [1] blanc */
} Plateau;

/* Mémoire d'un coup, pour pouvoir l'annuler (IA à plateau unique) */
typedef struct {
    Coup coup;
    char joueur;
    int nb_retournes;
    Coup retournes[NB_CASES];
    int stock_utilise;        /* Indice de la réserve dans laquelle le pion a été pris */
} Annulation;

void plateau_initialiser(Plateau *p, int depart);

char adversaire(char joueur);
int indice_joueur(char joueur);   /* 0 pour NOIR, 1 pour BLANC */

int dans_plateau(int lig, int col);
int compter_pions(const Plateau *p, char joueur);
int nb_cases_vides(const Plateau *p);

/* Renvoie 1 si on est encore dans la phase de pose libre des 4 pions centraux */
int phase_depart(const Plateau *p);

/* Renvoie 1 si le joueur peut poser un pion en (lig, col) */
int coup_valide(const Plateau *p, char joueur, int lig, int col);

/* Remplit coups[] avec les coups possibles du joueur (au plus NB_CASES).
   Renvoie leur nombre */
int coups_possibles(const Plateau *p, char joueur, Coup coups[]);

/* Renvoie 1 si le joueur a au moins un coup possible */
int peut_jouer(const Plateau *p, char joueur);

/* Joue un coup supposé valide : pose le pion et retourne les pions encadrés.
   Si annulation n'est pas NULL, elle reçoit de quoi annuler le coup.
   Renvoie le nombre de pions retournés */
int jouer_coup(Plateau *p, char joueur, int lig, int col, Annulation *annulation);

/* Remet le plateau dans l'état d'avant le coup */
void annuler_coup(Plateau *p, const Annulation *annulation);

/* La partie est terminée quand aucun des deux joueurs ne peut jouer */
int partie_terminee(const Plateau *p);

/* Renvoie 1 si la partie est terminée et que l'un des joueurs a gagné
   (gagnant reçoit NOIR ou BLANC), 0 sinon (partie en cours ou égalité) */
int position_gagnante(const Plateau *p, char *gagnant);

/* Conversion entre (lig, col) et la notation "D3" */
void coup_vers_texte(Coup c, char texte[4]);
int texte_vers_coup(const char *texte, Coup *c);

#endif
