#include <stdio.h>
#include <stdlib.h>
#include "ia.h"

#define INFINI 1000000

/* Compteurs de la recherche en cours */
static long compteur_noeuds = 0;
static long compteur_memoire = 0;

/* ========================================================================= */
/* Fonctions d'évaluation                                                     */
/* ========================================================================= */

/* Valeur stratégique de chaque case : les coins sont précieux (on ne peut
   plus les reprendre), les cases voisines des coins sont dangereuses (elles
   offrent le coin à l'adversaire), les bords sont plutôt bons. */
static const int POIDS[TAILLE][TAILLE] = {
    {100, -20, 10,  5,  5, 10, -20, 100},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    { 10,  -2,  1,  1,  1,  1,  -2,  10},
    {  5,  -2,  1,  0,  0,  1,  -2,   5},
    {  5,  -2,  1,  0,  0,  1,  -2,   5},
    { 10,  -2,  1,  1,  1,  1,  -2,  10},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    {100, -20, 10,  5,  5, 10, -20, 100}
};

/* Valeur d'une position terminale : gagner l'emporte sur tout le reste */
static int evaluation_finale(const Plateau *p, char joueur) {
    int diff = compter_pions(p, joueur) - compter_pions(p, adversaire(joueur));
    if (diff > 0) return VICTOIRE + diff;
    if (diff < 0) return -VICTOIRE + diff;
    return 0;
}

static int nb_coups(const Plateau *p, char joueur) {
    Coup coups[NB_CASES];
    return coups_possibles(p, joueur, coups);
}

/* Étape 3 : seulement le nombre de pions */
int evaluation_pions(const Plateau *p, char joueur) {
    return compter_pions(p, joueur) - compter_pions(p, adversaire(joueur));
}

/* Compte les alignements de longueur n (horizontaux, verticaux et
   diagonaux) formés uniquement de pions du joueur */
static int compter_alignements(const Plateau *p, char joueur, int n) {
    static const int DL[4] = {0, 1, 1, 1};
    static const int DC[4] = {1, 0, 1, -1};
    int i, j, d, k, total = 0, complet;

    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            for (d = 0; d < 4; d++) {
                complet = 1;
                for (k = 0; k < n && complet; k++) {
                    int l = i + k * DL[d], c = j + k * DC[d];
                    complet = dans_plateau(l, c) && p->cases[l][c] == joueur;
                }
                total += complet;
            }
        }
    }
    return total;
}

/* Étape 4 : pions + ensembles de 2 et de 3 pions consécutifs */
int evaluation_groupes(const Plateau *p, char joueur) {
    char adv = adversaire(joueur);
    return evaluation_pions(p, joueur)
         + 2 * (compter_alignements(p, joueur, 2) - compter_alignements(p, adv, 2))
         + 3 * (compter_alignements(p, joueur, 3) - compter_alignements(p, adv, 3));
}

/* Étape 5 : pondération des régions du plateau et mobilité */
int evaluation_ponderee(const Plateau *p, char joueur) {
    char adv = adversaire(joueur);
    int i, j, score = 0, mobilite_j, mobilite_a;

    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            if (p->cases[i][j] == joueur) score += POIDS[i][j];
            else if (p->cases[i][j] == adv) score -= POIDS[i][j];
        }
    }
    mobilite_j = nb_coups(p, joueur);
    mobilite_a = nb_coups(p, adv);
    score += 8 * (mobilite_j - mobilite_a);

    /* En fin de partie, seul le nombre de pions compte vraiment */
    if (nb_cases_vides(p) <= 10) {
        score += 10 * evaluation_pions(p, joueur);
    }
    return score;
}

/* Étape 7 : version plus fine. Les cases voisines d'un coin ne sont
   pénalisées que si le coin est encore libre, et on pénalise les pions
   "frontière" (au contact d'une case vide), qui donnent des coups à
   l'adversaire. */
int evaluation_expert(const Plateau *p, char joueur) {
    static const int COINS[4][2] = {{0, 0}, {0, 7}, {7, 0}, {7, 7}};
    static const int DL[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int DC[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    char adv = adversaire(joueur);
    int i, j, k, d, poids, score = 0, frontiere = 0, vides = nb_cases_vides(p);
    int mobilite_j, mobilite_a;

    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            char c = p->cases[i][j];
            if (c == VIDE) {
                continue;
            }
            poids = POIDS[i][j];
            /* Case voisine d'un coin déjà occupé : plus aucun danger */
            for (k = 0; k < 4; k++) {
                int ci = COINS[k][0], cj = COINS[k][1];
                if (abs(i - ci) <= 1 && abs(j - cj) <= 1 && (i != ci || j != cj)
                    && p->cases[ci][cj] != VIDE) {
                    poids = 5;
                }
            }
            /* Pion frontière */
            for (d = 0; d < 8; d++) {
                int l = i + DL[d], col = j + DC[d];
                if (dans_plateau(l, col) && p->cases[l][col] == VIDE) {
                    frontiere += (c == joueur) ? 1 : -1;
                    break;
                }
            }
            score += (c == joueur) ? poids : -poids;
        }
    }
    mobilite_j = nb_coups(p, joueur);
    mobilite_a = nb_coups(p, adv);
    score += 8 * (mobilite_j - mobilite_a);
    score -= 4 * frontiere;
    if (vides <= 14) {
        score += (16 - vides) * evaluation_pions(p, joueur);
    }
    return score;
}

int evaluation_plateau(const Plateau *p, char joueur, int niveau) {
    if (partie_terminee(p)) {
        return evaluation_finale(p, joueur);
    }
    switch (niveau) {
    case 2: return evaluation_pions(p, joueur);
    case 3: return evaluation_groupes(p, joueur);
    case 4:
    case 5: return evaluation_ponderee(p, joueur);
    default: return evaluation_expert(p, joueur);
    }
}

/* ========================================================================= */
/* Arbre de jeu (niveaux 2 et 3) : construit en entier, puis minimax          */
/* ========================================================================= */

static Noeud *nouveau_noeud(const Plateau *p, char trait, Coup coup) {
    Noeud *n = malloc(sizeof(Noeud));
    if (n == NULL) {
        fprintf(stderr, "Mémoire insuffisante pour l'arbre de jeu\n");
        exit(EXIT_FAILURE);
    }
    n->plateau = *p;
    n->trait = trait;
    n->coup = coup;
    n->valeur = 0;
    n->premier_fils = NULL;
    n->frere = NULL;
    compteur_memoire++;
    return n;
}

/* Ajoute les fils d'un nœud : un par coup possible, ou un seul fils
   "passe" si le joueur est bloqué mais pas son adversaire */
static void developper(Noeud *n) {
    Coup coups[NB_CASES], passe;
    Noeud *fils, *dernier = NULL;
    Plateau copie;
    int i, nb = coups_possibles(&n->plateau, n->trait, coups);

    if (nb == 0) {
        if (peut_jouer(&n->plateau, adversaire(n->trait))) {
            passe.lig = COUP_PASSE_LIG;
            passe.col = 0;
            n->premier_fils = nouveau_noeud(&n->plateau, adversaire(n->trait), passe);
        }
        return;
    }
    for (i = 0; i < nb; i++) {
        copie = n->plateau;
        jouer_coup(&copie, n->trait, coups[i].lig, coups[i].col, NULL);
        fils = nouveau_noeud(&copie, adversaire(n->trait), coups[i]);
        if (dernier == NULL) {
            n->premier_fils = fils;
        } else {
            dernier->frere = fils;
        }
        dernier = fils;
    }
}

static void construire(Noeud *n, int profondeur) {
    Noeud *f;
    if (profondeur <= 0) {
        return;
    }
    developper(n);
    for (f = n->premier_fils; f != NULL; f = f->frere) {
        construire(f, profondeur - 1);
    }
}

Noeud *creer_arbre_position(const Plateau *p, char trait, int profondeur) {
    Coup aucun;
    Noeud *racine;
    aucun.lig = COUP_PASSE_LIG;
    aucun.col = 0;
    racine = nouveau_noeud(p, trait, aucun);
    construire(racine, profondeur);
    return racine;
}

void liberer_arbre(Noeud *racine) {
    Noeud *f, *suivant;
    if (racine == NULL) {
        return;
    }
    for (f = racine->premier_fils; f != NULL; f = suivant) {
        suivant = f->frere;
        liberer_arbre(f);
    }
    free(racine);
}

long taille_arbre(const Noeud *racine) {
    const Noeud *f;
    long n = 1;
    for (f = racine->premier_fils; f != NULL; f = f->frere) {
        n += taille_arbre(f);
    }
    return n;
}

int minimax_arbre(Noeud *n, char ia, int niveau) {
    Noeud *f;
    int v, maximiser = (n->trait == ia);

    compteur_noeuds++;
    if (n->premier_fils == NULL) {
        n->valeur = evaluation_plateau(&n->plateau, ia, niveau);
        return n->valeur;
    }
    n->valeur = maximiser ? -INFINI : INFINI;
    for (f = n->premier_fils; f != NULL; f = f->frere) {
        v = minimax_arbre(f, ia, niveau);
        if (maximiser ? v > n->valeur : v < n->valeur) {
            n->valeur = v;
        }
    }
    return n->valeur;
}

/* ========================================================================= */
/* Alpha-bêta sur un arbre (niveau 4) : les fils sont créés au fur et à       */
/* mesure, les branches élaguées ne sont donc jamais construites              */
/* ========================================================================= */

static int alphabeta_arbre(Noeud *n, int profondeur, int alpha, int beta, char ia, int niveau) {
    Noeud *f;
    int v, maximiser = (n->trait == ia);
    Coup coups[NB_CASES], passe;
    Plateau copie;
    int i, nb;
    Noeud *dernier = NULL;

    compteur_noeuds++;
    if (profondeur == 0 || partie_terminee(&n->plateau)) {
        n->valeur = evaluation_plateau(&n->plateau, ia, niveau);
        return n->valeur;
    }
    nb = coups_possibles(&n->plateau, n->trait, coups);
    if (nb == 0) {
        passe.lig = COUP_PASSE_LIG;
        passe.col = 0;
        n->premier_fils = nouveau_noeud(&n->plateau, adversaire(n->trait), passe);
        n->valeur = alphabeta_arbre(n->premier_fils, profondeur - 1, alpha, beta, ia, niveau);
        return n->valeur;
    }

    n->valeur = maximiser ? -INFINI : INFINI;
    for (i = 0; i < nb; i++) {
        copie = n->plateau;
        jouer_coup(&copie, n->trait, coups[i].lig, coups[i].col, NULL);
        f = nouveau_noeud(&copie, adversaire(n->trait), coups[i]);
        if (dernier == NULL) n->premier_fils = f; else dernier->frere = f;
        dernier = f;

        v = alphabeta_arbre(f, profondeur - 1, alpha, beta, ia, niveau);
        if (maximiser) {
            if (v > n->valeur) n->valeur = v;
            if (v > alpha) alpha = v;
        } else {
            if (v < n->valeur) n->valeur = v;
            if (v < beta) beta = v;
        }
        if (alpha >= beta) {
            break; /* Élagage : les coups suivants ne sont pas explorés */
        }
    }
    return n->valeur;
}

/* ========================================================================= */
/* Alpha-bêta sur un plateau unique (niveaux 5 et 6) : on joue puis on        */
/* annule les coups, aucun arbre n'est conservé en mémoire                    */
/* ========================================================================= */

/* Trie les coups du plus prometteur au moins prometteur : alpha-bêta élague
   d'autant plus que les bons coups sont examinés en premier */
static void trier_coups(Coup coups[], int nb) {
    int i, j;
    Coup tmp;
    for (i = 1; i < nb; i++) {
        tmp = coups[i];
        for (j = i; j > 0 && POIDS[coups[j - 1].lig][coups[j - 1].col] < POIDS[tmp.lig][tmp.col]; j--) {
            coups[j] = coups[j - 1];
        }
        coups[j] = tmp;
    }
}

static int alphabeta(Plateau *p, char trait, int profondeur, int alpha, int beta, char ia, int niveau) {
    Coup coups[NB_CASES];
    Annulation annulation;
    int i, nb, v, meilleur, maximiser = (trait == ia);

    compteur_noeuds++;
    nb = coups_possibles(p, trait, coups);
    if (nb == 0) {
        if (!peut_jouer(p, adversaire(trait))) {
            return evaluation_finale(p, ia); /* Partie terminée */
        }
        if (profondeur == 0) {
            return evaluation_plateau(p, ia, niveau);
        }
        return alphabeta(p, adversaire(trait), profondeur - 1, alpha, beta, ia, niveau);
    }
    if (profondeur == 0) {
        return evaluation_plateau(p, ia, niveau);
    }

    trier_coups(coups, nb);
    meilleur = maximiser ? -INFINI : INFINI;
    for (i = 0; i < nb; i++) {
        jouer_coup(p, trait, coups[i].lig, coups[i].col, &annulation);
        v = alphabeta(p, adversaire(trait), profondeur - 1, alpha, beta, ia, niveau);
        annuler_coup(p, &annulation);

        if (maximiser) {
            if (v > meilleur) meilleur = v;
            if (v > alpha) alpha = v;
        } else {
            if (v < meilleur) meilleur = v;
            if (v < beta) beta = v;
        }
        if (alpha >= beta) {
            break;
        }
    }
    return meilleur;
}

/* ========================================================================= */
/* Choix du coup                                                              */
/* ========================================================================= */

int profondeur_par_defaut(int niveau) {
    switch (niveau) {
    case 2: return 2;
    case 3: return 4;
    case 4: return 5;
    case 5: return 7;
    case 6: return 8;
    default: return 1;
    }
}

const char *nom_niveau(int niveau) {
    switch (niveau) {
    case 1: return "Hasard";
    case 2: return "Débutant";
    case 3: return "Intermédiaire";
    case 4: return "Confirmé";
    case 5: return "Avancé";
    case 6: return "Expert";
    default: return "?";
    }
}

/* Choisit au hasard parmi les meilleurs coups ex aequo, pour varier les parties */
static Coup choisir_parmi_meilleurs(const Coup coups[], const int valeurs[], int nb, int *valeur) {
    int i, meilleur = -INFINI, nb_ex_aequo = 0, choix;
    for (i = 0; i < nb; i++) {
        if (valeurs[i] > meilleur) {
            meilleur = valeurs[i];
            nb_ex_aequo = 1;
        } else if (valeurs[i] == meilleur) {
            nb_ex_aequo++;
        }
    }
    choix = rand() % nb_ex_aequo;
    *valeur = meilleur;
    for (i = 0; i < nb; i++) {
        if (valeurs[i] == meilleur && choix-- == 0) {
            return coups[i];
        }
    }
    return coups[0];
}

/* Niveaux 2 et 3 : arbre complet puis minimax */
static Coup coup_par_arbre(const Plateau *p, char joueur, int niveau, int profondeur, int *valeur) {
    Coup coups[NB_CASES], choix;
    int valeurs[NB_CASES];
    int nb = 0;
    Noeud *racine = creer_arbre_position(p, joueur, profondeur), *f;

    minimax_arbre(racine, joueur, niveau);
    for (f = racine->premier_fils; f != NULL; f = f->frere) {
        coups[nb] = f->coup;
        valeurs[nb] = f->valeur;
        nb++;
    }
    choix = choisir_parmi_meilleurs(coups, valeurs, nb, valeur);
    liberer_arbre(racine);
    return choix;
}

/* Niveau 4 : alpha-bêta sur un arbre construit au fur et à mesure */
static Coup coup_par_arbre_alphabeta(const Plateau *p, char joueur, int profondeur, int *valeur) {
    Coup coups[NB_CASES], choix;
    int valeurs[NB_CASES];
    int nb = 0, alpha = -INFINI, v;
    Plateau copie;
    Coup possibles[NB_CASES];
    int i, n = coups_possibles(p, joueur, possibles);
    Noeud *racine, *f, *dernier = NULL;
    Coup aucun;

    aucun.lig = COUP_PASSE_LIG;
    aucun.col = 0;
    racine = nouveau_noeud(p, joueur, aucun);
    for (i = 0; i < n; i++) {
        copie = *p;
        jouer_coup(&copie, joueur, possibles[i].lig, possibles[i].col, NULL);
        f = nouveau_noeud(&copie, adversaire(joueur), possibles[i]);
        if (dernier == NULL) racine->premier_fils = f; else dernier->frere = f;
        dernier = f;
        /* Fenêtre (alpha - 1, +inf) : on garde les coups ex aequo */
        v = alphabeta_arbre(f, profondeur - 1, alpha - 1, INFINI, joueur, 4);
        if (v > alpha) alpha = v;
        coups[nb] = possibles[i];
        valeurs[nb] = v;
        nb++;
    }
    choix = choisir_parmi_meilleurs(coups, valeurs, nb, valeur);
    liberer_arbre(racine);
    return choix;
}

/* Niveaux 5 et 6 : alpha-bêta sur un plateau unique */
static Coup coup_par_alphabeta(const Plateau *p, char joueur, int niveau, int profondeur, int *valeur, int *atteinte) {
    Coup coups[NB_CASES], meilleur_coup;
    int valeurs[NB_CASES];
    int nb = coups_possibles(p, joueur, coups);
    int i, j, prof, debut, alpha, tmpv;
    Coup tmpc;
    Plateau travail = *p;
    Annulation annulation;

    trier_coups(coups, nb);
    /* Niveau 6 : si la fin est proche, on calcule la partie jusqu'au bout */
    if (niveau == 6 && nb_cases_vides(p) <= 14) {
        profondeur = nb_cases_vides(p);
    }
    /* Niveau 6 : approfondissement itératif, en replaçant en tête les
       meilleurs coups de l'itération précédente */
    debut = (niveau == 6) ? 1 : profondeur;

    for (prof = debut; prof <= profondeur; prof++) {
        alpha = -INFINI;
        for (i = 0; i < nb; i++) {
            jouer_coup(&travail, joueur, coups[i].lig, coups[i].col, &annulation);
            valeurs[i] = alphabeta(&travail, adversaire(joueur), prof - 1, alpha - 1, INFINI, joueur, niveau);
            annuler_coup(&travail, &annulation);
            if (valeurs[i] > alpha) alpha = valeurs[i];
        }
        /* Tri des coups par valeur décroissante pour l'itération suivante */
        for (i = 1; i < nb; i++) {
            tmpc = coups[i];
            tmpv = valeurs[i];
            for (j = i; j > 0 && valeurs[j - 1] < tmpv; j--) {
                coups[j] = coups[j - 1];
                valeurs[j] = valeurs[j - 1];
            }
            coups[j] = tmpc;
            valeurs[j] = tmpv;
        }
        *atteinte = prof;
        /* Inutile de chercher plus loin si la victoire est assurée */
        if (valeurs[0] >= VICTOIRE) {
            break;
        }
    }
    meilleur_coup = choisir_parmi_meilleurs(coups, valeurs, nb, valeur);
    return meilleur_coup;
}

Coup coup_ordinateur(const Plateau *p, char joueur, int niveau, int profondeur, Statistiques *stats) {
    Coup coups[NB_CASES], choix;
    int nb, valeur = 0, atteinte;

    compteur_noeuds = 0;
    compteur_memoire = 0;
    if (profondeur <= 0) {
        profondeur = profondeur_par_defaut(niveau);
    }
    if (profondeur > PROFONDEUR_MAX) {
        profondeur = PROFONDEUR_MAX;
    }
    if (niveau == 2) {
        profondeur = 2; /* Étape 3 : profondeur 2 imposée */
    }
    if (niveau == 3 && profondeur > 6) {
        profondeur = 6; /* Arbre complet : au-delà, il ne tiendrait plus en mémoire */
    }
    atteinte = profondeur;

    nb = coups_possibles(p, joueur, coups);
    if (nb == 0) {
        choix.lig = COUP_PASSE_LIG;
        choix.col = 0;
    } else if (nb == 1 || niveau <= 1) {
        choix = coups[rand() % nb];
        atteinte = niveau <= 1 ? 0 : 1;
    } else if (niveau <= 3) {
        choix = coup_par_arbre(p, joueur, niveau, profondeur, &valeur);
    } else if (niveau == 4) {
        choix = coup_par_arbre_alphabeta(p, joueur, profondeur, &valeur);
    } else {
        choix = coup_par_alphabeta(p, joueur, niveau, profondeur, &valeur, &atteinte);
    }

    if (stats != NULL) {
        stats->noeuds = compteur_noeuds;
        stats->noeuds_en_memoire = compteur_memoire;
        stats->valeur = valeur;
        stats->profondeur = atteinte;
    }
    return choix;
}
