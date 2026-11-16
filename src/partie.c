#include <stdio.h>
#include <string.h>
#include "partie.h"

void partie_initialiser(Partie *partie, int depart, const Joueur *noir, const Joueur *blanc, int profondeur) {
    plateau_initialiser(&partie->plateau, depart);
    partie->joueurs[0] = *noir;
    partie->joueurs[1] = *blanc;
    partie->joueurs[0].temps_ms = 0;
    partie->joueurs[1].temps_ms = 0;
    partie->trait = NOIR; /* Les noirs commencent */
    partie->dernier_coup.lig = -1;
    partie->dernier_coup.col = -1;
    partie->nb_coups = 0;
    partie->profondeur = profondeur;
    partie->a_passe = 0;
}

Joueur *joueur_courant(Partie *partie) {
    return &partie->joueurs[indice_joueur(partie->trait)];
}

int partie_jouer(Partie *partie, Coup coup) {
    int retournes = jouer_coup(&partie->plateau, partie->trait, coup.lig, coup.col, NULL);
    char adv = adversaire(partie->trait);

    partie->dernier_coup = coup;
    partie->nb_coups++;
    partie->a_passe = 0;
    if (peut_jouer(&partie->plateau, adv)) {
        partie->trait = adv;
    } else if (peut_jouer(&partie->plateau, partie->trait)) {
        partie->a_passe = 1; /* L'adversaire est bloqué : il passe son tour */
    }
    return retournes;
}

int partie_finie(const Partie *partie) {
    return partie_terminee(&partie->plateau);
}

static void ecrire_joueur(FILE *f, const Joueur *j) {
    fprintf(f, "%d %d %d %ld %s\n", j->type, j->niveau, j->avatar, j->temps_ms, j->nom);
}

static int lire_joueur(FILE *f, Joueur *j) {
    char ligne[128];
    int n = 0;
    if (fgets(ligne, sizeof(ligne), f) == NULL
        || sscanf(ligne, "%d %d %d %ld %n", &j->type, &j->niveau, &j->avatar, &j->temps_ms, &n) != 4) {
        return 0;
    }
    ligne[strcspn(ligne, "\r\n")] = '\0';
    strncpy(j->nom, ligne + n, LONGUEUR_NOM - 1);
    j->nom[LONGUEUR_NOM - 1] = '\0';
    return (j->type == HUMAIN || j->type == ORDINATEUR) && j->niveau >= 1 && j->niveau <= 6;
}

int sauvegarder_partie(const Partie *partie, const char *fichier) {
    FILE *f = fopen(fichier, "w");
    int i;
    if (f == NULL) {
        return -1;
    }
    fprintf(f, "OTHELLO 1\n");
    fprintf(f, "%c %d %d %d %d %d %d %d %d\n", partie->trait, partie->plateau.depart,
            partie->plateau.stock[0], partie->plateau.stock[1], partie->nb_coups,
            partie->profondeur, partie->dernier_coup.lig, partie->dernier_coup.col, partie->a_passe);
    ecrire_joueur(f, &partie->joueurs[0]);
    ecrire_joueur(f, &partie->joueurs[1]);
    for (i = 0; i < TAILLE; i++) {
        fprintf(f, "%.8s\n", partie->plateau.cases[i]);
    }
    return fclose(f) == 0 ? 0 : -1;
}

int charger_partie(Partie *partie, const char *fichier) {
    FILE *f = fopen(fichier, "r");
    char ligne[128];
    int i, j, ok;
    Partie lue;

    if (f == NULL) {
        return -1;
    }
    ok = fgets(ligne, sizeof(ligne), f) != NULL && strncmp(ligne, "OTHELLO 1", 9) == 0;
    ok = ok && fgets(ligne, sizeof(ligne), f) != NULL
         && sscanf(ligne, " %c %d %d %d %d %d %d %d %d", &lue.trait, &lue.plateau.depart,
                   &lue.plateau.stock[0], &lue.plateau.stock[1], &lue.nb_coups, &lue.profondeur,
                   &lue.dernier_coup.lig, &lue.dernier_coup.col, &lue.a_passe) == 9;
    ok = ok && lire_joueur(f, &lue.joueurs[0]) && lire_joueur(f, &lue.joueurs[1]);
    for (i = 0; ok && i < TAILLE; i++) {
        ok = fgets(ligne, sizeof(ligne), f) != NULL && strlen(ligne) >= TAILLE;
        for (j = 0; ok && j < TAILLE; j++) {
            ok = ligne[j] == VIDE || ligne[j] == NOIR || ligne[j] == BLANC;
            lue.plateau.cases[i][j] = ligne[j];
        }
    }
    fclose(f);
    if (!ok || (lue.trait != NOIR && lue.trait != BLANC)) {
        return -1;
    }
    *partie = lue;
    return 0;
}

int sauvegarde_existe(const char *fichier) {
    FILE *f = fopen(fichier, "r");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}
