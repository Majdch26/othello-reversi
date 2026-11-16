/* Tests unitaires des règles et de l'IA (sans interface graphique).
   Lancement : make test */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plateau.h"
#include "partie.h"
#include "ia.h"

static int nb_tests = 0, nb_echecs = 0;

#define VERIFIER(c) verifier((c), #c, __LINE__)

static void verifier(int ok, const char *texte, int ligne) {
    nb_tests++;
    if (!ok) {
        nb_echecs++;
        printf("  ECHEC ligne %d : %s\n", ligne, texte);
    }
}

/* Construit un plateau à partir de 8 chaînes de 8 caractères */
static void charger(Plateau *p, const char *lignes[TAILLE]) {
    int i, j;
    plateau_initialiser(p, DEPART_OTHELLO);
    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            p->cases[i][j] = lignes[i][j];
        }
    }
}

static void test_depart(void) {
    Plateau p;
    Coup coups[NB_CASES];
    printf("Départs Othello et Reversi\n");

    plateau_initialiser(&p, DEPART_OTHELLO);
    VERIFIER(compter_pions(&p, NOIR) == 2 && compter_pions(&p, BLANC) == 2);
    VERIFIER(coups_possibles(&p, NOIR, coups) == 4);
    VERIFIER(coup_valide(&p, NOIR, 2, 3));      /* D3 */
    VERIFIER(!coup_valide(&p, NOIR, 0, 0));
    VERIFIER(!phase_depart(&p));

    plateau_initialiser(&p, DEPART_REVERSI);
    VERIFIER(nb_cases_vides(&p) == 64);
    VERIFIER(phase_depart(&p));
    VERIFIER(coups_possibles(&p, NOIR, coups) == 4); /* Les 4 cases centrales */
    VERIFIER(coup_valide(&p, NOIR, 3, 3) && !coup_valide(&p, NOIR, 2, 2));
    jouer_coup(&p, NOIR, 3, 3, NULL);
    jouer_coup(&p, BLANC, 3, 4, NULL);
    jouer_coup(&p, NOIR, 4, 4, NULL);
    VERIFIER(phase_depart(&p));
    jouer_coup(&p, BLANC, 4, 3, NULL);
    VERIFIER(!phase_depart(&p));
    VERIFIER(compter_pions(&p, NOIR) == 2 && compter_pions(&p, BLANC) == 2); /* Aucun retournement */
    VERIFIER(p.stock[0] == 30 && p.stock[1] == 30);
}

static void test_retournements(void) {
    Plateau p;
    Annulation a;
    Plateau avant;
    const char *grille[TAILLE] = {
        "BBB.....",
        "BNN.....",
        "BN.N....",
        "...N....",
        "....B...",
        "........",
        "........",
        "........"
    };
    printf("Retournements dans plusieurs directions\n");
    charger(&p, grille);
    /* Blanc en C3 (ligne 2, colonne 2) encadre C2 (vers le haut),
       B2 (diagonale haut-gauche), B3 (vers la gauche) et D4 (diagonale
       bas-droite, jusqu'au pion blanc en E5) */
    avant = p;
    VERIFIER(coup_valide(&p, BLANC, 2, 2));
    VERIFIER(jouer_coup(&p, BLANC, 2, 2, &a) == 4);
    VERIFIER(p.cases[1][2] == BLANC && p.cases[1][1] == BLANC && p.cases[2][1] == BLANC && p.cases[3][3] == BLANC);
    VERIFIER(p.cases[2][3] == NOIR); /* D3 n'est pas encadré */
    annuler_coup(&p, &a);
    VERIFIER(memcmp(&p, &avant, sizeof(Plateau)) == 0);
}

static void test_passe_et_fin(void) {
    Plateau p;
    char gagnant = 0;
    const char *bloque[TAILLE] = {
        "NNNNNNNN",
        "NNNNNNNN",
        "NNNNNNNN",
        "NNNNNNNN",
        "NNNNNNNB",
        "NNNNNNN.",
        "NNNNNNNN",
        "NNNNNNNN"
    };
    const char *plein[TAILLE] = {
        "NNNNNNNN", "NNNNNNNN", "NNNNNNNN", "NNNNNNNN",
        "BBBBBBBB", "BBBBBBBB", "BBBBBBBB", "BBBBBBBB"
    };
    printf("Passe et fin de partie\n");
    charger(&p, bloque);
    VERIFIER(!peut_jouer(&p, BLANC));       /* Blanc ne peut rien encadrer */
    VERIFIER(peut_jouer(&p, NOIR));         /* Noir peut jouer en H6 */
    VERIFIER(!partie_terminee(&p));
    jouer_coup(&p, NOIR, 5, 7, NULL);
    VERIFIER(partie_terminee(&p));          /* Fin avant que le plateau soit plein */
    VERIFIER(position_gagnante(&p, &gagnant) && gagnant == NOIR);

    charger(&p, plein);
    VERIFIER(partie_terminee(&p));
    VERIFIER(!position_gagnante(&p, &gagnant)); /* 32 - 32 : égalité */
}

static void test_partie_et_passe(void) {
    Partie partie;
    Joueur a, b;
    Coup c;
    const char *bloque[TAILLE] = {
        "NNNNNNNN", "NNNNNNNN", "NNNNNNNN", "NNNNNNNN",
        "NNNNNNNB", "NNNNNN..", "NNNNNNNN", "NNNNNNNN"
    };
    printf("Gestion du tour et des passes\n");
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.niveau = b.niveau = 1;
    partie_initialiser(&partie, DEPART_OTHELLO, &a, &b, 0);
    charger(&partie.plateau, bloque);
    partie.trait = NOIR;
    c.lig = 5;
    c.col = 7;
    partie_jouer(&partie, c);              /* Noir H6 : Blanc reste bloqué */
    VERIFIER(partie.trait == NOIR || partie_finie(&partie));
}

static void test_notation(void) {
    Coup c;
    char t[4];
    printf("Notation des coups\n");
    VERIFIER(texte_vers_coup("D3", &c) && c.col == 3 && c.lig == 2);
    VERIFIER(texte_vers_coup("d3", &c) && c.col == 3 && c.lig == 2);
    VERIFIER(texte_vers_coup("3 d", &c) && c.col == 3 && c.lig == 2);
    VERIFIER(!texte_vers_coup("I3", &c));
    VERIFIER(!texte_vers_coup("D9", &c));
    VERIFIER(!texte_vers_coup("bonjour", &c));
    c.lig = 7;
    c.col = 7;
    coup_vers_texte(c, t);
    VERIFIER(strcmp(t, "H8") == 0);
}

static void test_ia_coups_valides(void) {
    int niveau, partie_n;
    printf("L'IA ne joue que des coups valides (6 niveaux, parties complètes)\n");
    for (niveau = NIVEAU_MIN; niveau <= NIVEAU_MAX; niveau++) {
        for (partie_n = 0; partie_n < 2; partie_n++) {
            Plateau p;
            char trait = NOIR;
            int ok = 1, coups = 0;
            plateau_initialiser(&p, partie_n == 0 ? DEPART_REVERSI : DEPART_OTHELLO);
            while (!partie_terminee(&p) && coups < 100) {
                Coup c;
                if (!peut_jouer(&p, trait)) {
                    trait = adversaire(trait);
                    continue;
                }
                /* Le niveau testé joue les Noirs, le hasard les Blancs */
                c = coup_ordinateur(&p, trait, trait == NOIR ? niveau : 1, niveau >= 5 ? 3 : 0, NULL);
                if (!coup_valide(&p, trait, c.lig, c.col)) {
                    ok = 0;
                    break;
                }
                jouer_coup(&p, trait, c.lig, c.col, NULL);
                trait = adversaire(trait);
                coups++;
            }
            VERIFIER(ok && partie_terminee(&p));
            VERIFIER(compter_pions(&p, NOIR) + compter_pions(&p, BLANC) + nb_cases_vides(&p) == 64);
        }
    }
}

static void test_ia_coin(void) {
    Plateau p;
    Coup c;
    int niveau;
    /* Noir peut prendre le coin A1 ou jouer ailleurs : les IA à partir du
       niveau 4 (évaluation pondérée) doivent prendre le coin */
    const char *grille[TAILLE] = {
        ".BBBBN..",
        "........",
        "........",
        "...BN...",
        "...NB...",
        "........",
        "........",
        "........"
    };
    printf("Les IA pondérées prennent le coin\n");
    charger(&p, grille);
    VERIFIER(coup_valide(&p, NOIR, 0, 0));
    for (niveau = 4; niveau <= 6; niveau++) {
        c = coup_ordinateur(&p, NOIR, niveau, 3, NULL);
        VERIFIER(c.lig == 0 && c.col == 0);
    }
}

static void test_arbre(void) {
    Plateau p;
    Noeud *arbre;
    printf("Arbre de jeu (pointeurs)\n");
    plateau_initialiser(&p, DEPART_OTHELLO);
    arbre = creer_arbre_position(&p, NOIR, 2);
    /* Racine + 4 coups noirs + 3 réponses blanches à chacun = 17 nœuds */
    VERIFIER(taille_arbre(arbre) == 17);
    liberer_arbre(arbre);

    arbre = creer_arbre_position(&p, NOIR, 0);
    VERIFIER(taille_arbre(arbre) == 1);
    liberer_arbre(arbre);
}

/* Minimax naïf servant de référence pour vérifier l'alpha-bêta */
static int minimax_reference(Plateau *p, char trait, int prof, char ia, int niveau) {
    Coup coups[NB_CASES];
    Annulation a;
    int i, nb = coups_possibles(p, trait, coups), v, meilleur;
    if (nb == 0) {
        if (!peut_jouer(p, adversaire(trait))) return evaluation_plateau(p, ia, niveau);
        if (prof == 0) return evaluation_plateau(p, ia, niveau);
        return minimax_reference(p, adversaire(trait), prof - 1, ia, niveau);
    }
    if (prof == 0) return evaluation_plateau(p, ia, niveau);
    meilleur = trait == ia ? -10000000 : 10000000;
    for (i = 0; i < nb; i++) {
        jouer_coup(p, trait, coups[i].lig, coups[i].col, &a);
        v = minimax_reference(p, adversaire(trait), prof - 1, ia, niveau);
        annuler_coup(p, &a);
        if (trait == ia ? v > meilleur : v < meilleur) meilleur = v;
    }
    return meilleur;
}

static void test_alphabeta_egal_minimax(void) {
    int partie_n, k, prof = 4;
    printf("Alpha-bêta donne la même valeur que minimax (niveaux 4 et 5)\n");
    srand(3);
    for (partie_n = 0; partie_n < 4; partie_n++) {
        Plateau p;
        char trait = NOIR;
        Coup coups[NB_CASES];
        Statistiques s4, s5;
        int reference, n;
        plateau_initialiser(&p, DEPART_OTHELLO);
        /* Position aléatoire de milieu de partie */
        for (k = 0; k < 14 + partie_n * 6 && !partie_terminee(&p); k++) {
            if (!peut_jouer(&p, trait)) { trait = adversaire(trait); continue; }
            n = coups_possibles(&p, trait, coups);
            n = rand() % n;
            jouer_coup(&p, trait, coups[n].lig, coups[n].col, NULL);
            trait = adversaire(trait);
        }
        if (!peut_jouer(&p, trait)) continue;
        /* La valeur de la racine = max sur les coups du minimax sur prof-1 */
        reference = -10000000;
        n = coups_possibles(&p, trait, coups);
        for (k = 0; k < n; k++) {
            Annulation a;
            int v;
            jouer_coup(&p, trait, coups[k].lig, coups[k].col, &a);
            v = minimax_reference(&p, adversaire(trait), prof - 1, trait, 5);
            annuler_coup(&p, &a);
            if (v > reference) reference = v;
        }
        coup_ordinateur(&p, trait, 4, prof, &s4);
        coup_ordinateur(&p, trait, 5, prof, &s5);
        VERIFIER(s4.valeur == reference);
        VERIFIER(s5.valeur == reference);
        VERIFIER(s5.noeuds_en_memoire == 0);   /* Niveau 5 : aucun arbre en mémoire */
        VERIFIER(s4.noeuds_en_memoire > 0);
    }
}

static void test_sauvegarde(void) {
    Partie partie, lue;
    Joueur h, o;
    Coup c;
    printf("Sauvegarde et chargement\n");
    memset(&h, 0, sizeof(h));
    memset(&o, 0, sizeof(o));
    h.type = HUMAIN; h.niveau = 1; strcpy(h.nom, "Majd Chamas"); h.avatar = 2;
    o.type = ORDINATEUR; o.niveau = 5; strcpy(o.nom, "Tor");
    partie_initialiser(&partie, DEPART_OTHELLO, &h, &o, 6);
    c.lig = 2; c.col = 3;
    partie_jouer(&partie, c);
    VERIFIER(sauvegarder_partie(&partie, "saves/test_partie.txt") == 0);
    memset(&lue, 0, sizeof(lue));
    VERIFIER(charger_partie(&lue, "saves/test_partie.txt") == 0);
    VERIFIER(memcmp(lue.plateau.cases, partie.plateau.cases, sizeof(partie.plateau.cases)) == 0);
    VERIFIER(lue.trait == BLANC && lue.nb_coups == 1 && lue.profondeur == 6);
    VERIFIER(strcmp(lue.joueurs[0].nom, "Majd Chamas") == 0 && lue.joueurs[0].avatar == 2);
    VERIFIER(lue.joueurs[1].type == ORDINATEUR && lue.joueurs[1].niveau == 5);
    VERIFIER(lue.plateau.stock[0] == partie.plateau.stock[0]);
    remove("saves/test_partie.txt");
    VERIFIER(charger_partie(&lue, "saves/inexistant.txt") == -1);
}

int main(void) {
    srand(42);
    test_depart();
    test_retournements();
    test_passe_et_fin();
    test_partie_et_passe();
    test_notation();
    test_arbre();
    test_ia_coin();
    test_alphabeta_egal_minimax();
    test_sauvegarde();
    test_ia_coups_valides();
    printf("\n%d tests, %d échec(s)\n", nb_tests, nb_echecs);
    return nb_echecs == 0 ? 0 : 1;
}
