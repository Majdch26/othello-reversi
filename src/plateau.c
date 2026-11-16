#include <ctype.h>
#include <string.h>
#include "plateau.h"

/* Les 8 directions : (variation de ligne, variation de colonne) */
static const int DL[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const int DC[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

void plateau_initialiser(Plateau *p, int depart) {
    int i, j;
    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            p->cases[i][j] = VIDE;
        }
    }
    p->depart = depart;
    p->stock[0] = PIONS_PAR_JOUEUR;
    p->stock[1] = PIONS_PAR_JOUEUR;

    if (depart == DEPART_OTHELLO) {
        /* d4 et e5 blancs, d5 et e4 noirs */
        p->cases[3][3] = BLANC;
        p->cases[4][4] = BLANC;
        p->cases[3][4] = NOIR;
        p->cases[4][3] = NOIR;
        p->stock[0] -= 2;
        p->stock[1] -= 2;
    }
}

char adversaire(char joueur) {
    return joueur == NOIR ? BLANC : NOIR;
}

int indice_joueur(char joueur) {
    return joueur == NOIR ? 0 : 1;
}

int dans_plateau(int lig, int col) {
    return lig >= 0 && lig < TAILLE && col >= 0 && col < TAILLE;
}

int compter_pions(const Plateau *p, char joueur) {
    int i, j, n = 0;
    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            n += p->cases[i][j] == joueur;
        }
    }
    return n;
}

int nb_cases_vides(const Plateau *p) {
    return compter_pions(p, VIDE);
}

static int est_case_centrale(int lig, int col) {
    return (lig == 3 || lig == 4) && (col == 3 || col == 4);
}

int phase_depart(const Plateau *p) {
    return p->depart == DEPART_REVERSI
        && (p->cases[3][3] == VIDE || p->cases[3][4] == VIDE
            || p->cases[4][3] == VIDE || p->cases[4][4] == VIDE);
}

/* Nombre de pions adverses encadrés dans la direction d en posant en (lig, col) */
static int encadres_direction(const Plateau *p, char joueur, int lig, int col, int d) {
    char adv = adversaire(joueur);
    int l = lig + DL[d], c = col + DC[d], n = 0;

    while (dans_plateau(l, c) && p->cases[l][c] == adv) {
        l += DL[d];
        c += DC[d];
        n++;
    }
    if (n > 0 && dans_plateau(l, c) && p->cases[l][c] == joueur) {
        return n;
    }
    return 0;
}

int coup_valide(const Plateau *p, char joueur, int lig, int col) {
    int d;

    if (!dans_plateau(lig, col) || p->cases[lig][col] != VIDE) {
        return 0;
    }
    if (phase_depart(p)) {
        /* Les 4 premiers pions se posent librement au centre */
        return est_case_centrale(lig, col);
    }
    for (d = 0; d < 8; d++) {
        if (encadres_direction(p, joueur, lig, col, d) > 0) {
            return 1;
        }
    }
    return 0;
}

int coups_possibles(const Plateau *p, char joueur, Coup coups[]) {
    int i, j, n = 0;
    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            if (coup_valide(p, joueur, i, j)) {
                coups[n].lig = i;
                coups[n].col = j;
                n++;
            }
        }
    }
    return n;
}

int peut_jouer(const Plateau *p, char joueur) {
    int i, j;
    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            if (coup_valide(p, joueur, i, j)) {
                return 1;
            }
        }
    }
    return 0;
}

int jouer_coup(Plateau *p, char joueur, int lig, int col, Annulation *annulation) {
    int d, k, n, total = 0, l, c, moi = indice_joueur(joueur), stock;
    int depart = phase_depart(p);

    /* Le pion est pris dans la réserve du joueur ; si elle est vide,
       l'adversaire lui en donne un (règle officielle) */
    stock = (p->stock[moi] > 0) ? moi : 1 - moi;
    if (p->stock[stock] > 0) {
        p->stock[stock]--;
    }
    if (annulation != NULL) {
        annulation->coup.lig = lig;
        annulation->coup.col = col;
        annulation->joueur = joueur;
        annulation->stock_utilise = stock;
    }

    if (!depart) {
        for (d = 0; d < 8; d++) {
            n = encadres_direction(p, joueur, lig, col, d);
            for (k = 1; k <= n; k++) {
                l = lig + k * DL[d];
                c = col + k * DC[d];
                p->cases[l][c] = joueur;
                if (annulation != NULL) {
                    annulation->retournes[total].lig = l;
                    annulation->retournes[total].col = c;
                }
                total++;
            }
        }
    }
    p->cases[lig][col] = joueur;
    if (annulation != NULL) {
        annulation->nb_retournes = total;
    }
    return total;
}

void annuler_coup(Plateau *p, const Annulation *a) {
    int k;
    char adv = adversaire(a->joueur);
    for (k = 0; k < a->nb_retournes; k++) {
        p->cases[a->retournes[k].lig][a->retournes[k].col] = adv;
    }
    p->cases[a->coup.lig][a->coup.col] = VIDE;
    p->stock[a->stock_utilise]++;
}

int partie_terminee(const Plateau *p) {
    return !peut_jouer(p, NOIR) && !peut_jouer(p, BLANC);
}

int position_gagnante(const Plateau *p, char *gagnant) {
    int noirs, blancs;
    if (!partie_terminee(p)) {
        return 0;
    }
    noirs = compter_pions(p, NOIR);
    blancs = compter_pions(p, BLANC);
    if (noirs == blancs) {
        return 0;
    }
    if (gagnant != NULL) {
        *gagnant = noirs > blancs ? NOIR : BLANC;
    }
    return 1;
}

void coup_vers_texte(Coup c, char texte[4]) {
    if (c.lig == COUP_PASSE_LIG) {
        strcpy(texte, "--");
        return;
    }
    texte[0] = (char)('A' + c.col);
    texte[1] = (char)('1' + c.lig);
    texte[2] = '\0';
}

int texte_vers_coup(const char *texte, Coup *c) {
    char lettre = 0;
    int chiffre = 0, i;

    /* Accepte "D3", "d3", "3D", "3 d", "D 3"... */
    for (i = 0; texte[i] != '\0'; i++) {
        if (isalpha((unsigned char)texte[i]) && lettre == 0) {
            lettre = (char)toupper((unsigned char)texte[i]);
        } else if (isdigit((unsigned char)texte[i]) && chiffre == 0) {
            chiffre = texte[i] - '0';
        } else if (!isspace((unsigned char)texte[i]) && texte[i] != ',') {
            return 0;
        }
    }
    if (lettre < 'A' || lettre > 'H' || chiffre < 1 || chiffre > 8) {
        return 0;
    }
    c->col = lettre - 'A';
    c->lig = chiffre - 1;
    return 1;
}
