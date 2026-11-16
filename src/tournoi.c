/* Tournoi entre les niveaux de l'IA (étape 7 : faire jouer deux programmes
   l'un contre l'autre). Chaque paire de niveaux joue plusieurs parties en
   alternant les couleurs ; les premiers coups sont tirés au hasard pour que
   les parties soient différentes.

     tournoi [-parties N] [-niveaux 1,3,5] [-depart reversi|othello] [-graine G] */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "plateau.h"
#include "ia.h"

#define COUPS_ALEATOIRES 4

/* Joue une partie. Renvoie (pions de noir - pions de blanc) */
static int jouer_partie(int niveau_noir, int niveau_blanc, int depart, double *temps_noir, double *temps_blanc) {
    Plateau p;
    char trait = NOIR;
    Coup coups[NB_CASES], c;
    int n, numero = 0;
    clock_t debut;

    plateau_initialiser(&p, depart);
    while (!partie_terminee(&p)) {
        if (!peut_jouer(&p, trait)) {
            trait = adversaire(trait);
            continue;
        }
        if (numero < COUPS_ALEATOIRES) {
            n = coups_possibles(&p, trait, coups);
            c = coups[rand() % n];
        } else {
            debut = clock();
            c = coup_ordinateur(&p, trait, trait == NOIR ? niveau_noir : niveau_blanc, 0, NULL);
            if (trait == NOIR) *temps_noir += (double)(clock() - debut) / CLOCKS_PER_SEC;
            else *temps_blanc += (double)(clock() - debut) / CLOCKS_PER_SEC;
        }
        jouer_coup(&p, trait, c.lig, c.col, NULL);
        trait = adversaire(trait);
        numero++;
    }
    return compter_pions(&p, NOIR) - compter_pions(&p, BLANC);
}

int main(int argc, char *argv[]) {
    int niveaux[NIVEAU_MAX], nb_niveaux = 0;
    int parties = 10, depart = DEPART_REVERSI, i, a, b, g, diff;
    int victoires[NIVEAU_MAX + 1][NIVEAU_MAX + 1];
    double temps[NIVEAU_MAX + 1];
    long coups_joues[NIVEAU_MAX + 1];
    unsigned int graine = (unsigned int)time(NULL);
    char *liste = NULL, *morceau;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-parties") == 0 && i + 1 < argc) parties = atoi(argv[++i]);
        else if (strcmp(argv[i], "-niveaux") == 0 && i + 1 < argc) liste = argv[++i];
        else if (strcmp(argv[i], "-graine") == 0 && i + 1 < argc) graine = (unsigned int)atoi(argv[++i]);
        else if (strcmp(argv[i], "-depart") == 0 && i + 1 < argc) {
            depart = strcmp(argv[++i], "othello") == 0 ? DEPART_OTHELLO : DEPART_REVERSI;
        } else {
            printf("Utilisation : %s [-parties N] [-niveaux 1,3,5] [-depart reversi|othello] [-graine G]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (liste == NULL) {
        for (i = NIVEAU_MIN; i <= NIVEAU_MAX; i++) niveaux[nb_niveaux++] = i;
    } else {
        for (morceau = strtok(liste, ","); morceau != NULL && nb_niveaux < NIVEAU_MAX; morceau = strtok(NULL, ",")) {
            int n = atoi(morceau);
            if (n >= NIVEAU_MIN && n <= NIVEAU_MAX) niveaux[nb_niveaux++] = n;
        }
    }
    if (parties < 1 || nb_niveaux < 2) {
        fprintf(stderr, "Il faut au moins 1 partie et 2 niveaux.\n");
        return EXIT_FAILURE;
    }
    srand(graine);
    memset(victoires, 0, sizeof(victoires));
    memset(temps, 0, sizeof(temps));
    memset(coups_joues, 0, sizeof(coups_joues));

    printf("Tournoi : %d parties par paire de niveaux, départ %s, graine %u\n\n",
           parties, depart == DEPART_OTHELLO ? "Othello" : "Reversi", graine);
    for (a = 0; a < nb_niveaux; a++) {
        for (b = a + 1; b < nb_niveaux; b++) {
            int na = niveaux[a], nb = niveaux[b], va = 0, vb = 0, nuls = 0;
            for (g = 0; g < parties; g++) {
                double tn = 0, tb = 0;
                /* Couleurs alternées : chaque niveau joue autant de fois avec les Noirs */
                int a_noir = (g % 2 == 0);
                diff = jouer_partie(a_noir ? na : nb, a_noir ? nb : na, depart, &tn, &tb);
                if (!a_noir) diff = -diff;
                if (diff > 0) va++; else if (diff < 0) vb++; else nuls++;
                temps[na] += a_noir ? tn : tb;
                temps[nb] += a_noir ? tb : tn;
                coups_joues[na] += 30;
                coups_joues[nb] += 30;
            }
            victoires[na][nb] = va;
            victoires[nb][na] = vb;
            printf("Niveau %d (%s) contre niveau %d (%s) : %d - %d", na, nom_niveau(na), nb, nom_niveau(nb), va, vb);
            if (nuls) printf(" (%d nul%s)", nuls, nuls > 1 ? "s" : "");
            printf("\n");
            fflush(stdout);
        }
    }

    printf("\nVictoires (ligne contre colonne) :\n       ");
    for (b = 0; b < nb_niveaux; b++) printf(" niv%d", niveaux[b]);
    printf("   total\n");
    for (a = 0; a < nb_niveaux; a++) {
        int total = 0;
        printf("niv%d   ", niveaux[a]);
        for (b = 0; b < nb_niveaux; b++) {
            if (a == b) printf("    -");
            else {
                printf(" %4d", victoires[niveaux[a]][niveaux[b]]);
                total += victoires[niveaux[a]][niveaux[b]];
            }
        }
        printf("   %3d / %d   (%.3f s par coup)\n", total, parties * (nb_niveaux - 1),
               coups_joues[niveaux[a]] ? temps[niveaux[a]] / coups_joues[niveaux[a]] : 0.0);
    }
    return EXIT_SUCCESS;
}
