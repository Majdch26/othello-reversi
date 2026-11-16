/* fileno() et isatty() sont des fonctions POSIX */
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include "console.h"
#include "partie.h"
#include "ia.h"

static int couleurs_actives = 0;

#define C(code) (couleurs_actives ? (code) : "")
#define RESET   C("\033[0m")
#define GRAS    C("\033[1m")
#define VERT    C("\033[32m")
#define JAUNE   C("\033[33m")
#define CYAN    C("\033[36m")
#define GRIS    C("\033[90m")
#define ROUGE   C("\033[31m")

/* Lit une ligne au clavier. Renvoie 0 en fin d'entrée */
static int lire_ligne(char *tampon, int taille) {
    if (fgets(tampon, taille, stdin) == NULL) {
        return 0;
    }
    tampon[strcspn(tampon, "\r\n")] = '\0';
    return 1;
}

static long millisecondes(void) {
    return (long)(clock() * 1000.0 / CLOCKS_PER_SEC);
}

static const char *nom_couleur(char c) {
    return c == NOIR ? "Noir" : "Blanc";
}

/* Affiche le plateau au format du sujet. Les coups possibles du joueur
   humain sont signalés par '+' et le dernier coup est mis en évidence */
static void afficher_plateau(const Partie *partie, int montrer_coups) {
    int i, j;
    const Plateau *p = &partie->plateau;

    printf("\n   ________________\n");
    for (i = 0; i < TAILLE; i++) {
        printf("%d | ", i + 1);
        for (j = 0; j < TAILLE; j++) {
            char c = p->cases[i][j];
            int dernier = partie->dernier_coup.lig == i && partie->dernier_coup.col == j;
            if (j > 0) {
                printf(" ");
            }
            if (c == NOIR) {
                printf("%s%sN%s", dernier ? JAUNE : "", GRAS, RESET);
            } else if (c == BLANC) {
                printf("%s%sB%s", dernier ? JAUNE : CYAN, GRAS, RESET);
            } else if (montrer_coups && coup_valide(p, partie->trait, i, j)) {
                printf("%s+%s", VERT, RESET);
            } else {
                printf("%s.%s", GRIS, RESET);
            }
        }
        printf("|\n");
    }
    printf("   ----------------\n");
    printf("    A B C D E F G H\n\n");
    printf("   Noir : %d pion(s)   Blanc : %d pion(s)\n", compter_pions(p, NOIR), compter_pions(p, BLANC));
}

static void afficher_regles(void) {
    printf("\n%s== Règles du Reversi ==%s\n\n", GRAS, RESET);
    printf("- Le plateau fait 8 x 8 cases. Les Noirs commencent.\n");
    printf("- Départ Reversi : les 4 premiers pions sont posés à tour de rôle sur\n"
           "  d4, d5, e4 ou e5, au choix (départ Othello : ils sont déjà placés).\n");
    printf("- Ensuite, chaque pion posé doit encadrer au moins un pion adverse, en\n"
           "  ligne, en colonne ou en diagonale, avec un autre pion de sa couleur.\n");
    printf("- Tous les pions encadrés sont retournés.\n");
    printf("- Un joueur qui ne peut pas jouer passe son tour.\n");
    printf("- La partie s'arrête quand plus personne ne peut jouer : celui qui a le\n"
           "  plus de pions gagne.\n\n");
    printf("Pour jouer, tapez la colonne puis la ligne (par exemple %sD3%s).\n"
           "Commandes : %saide%s (coups possibles), %ssauver%s, %squitter%s.\n\n",
           GRAS, RESET, GRAS, RESET, GRAS, RESET, GRAS, RESET);
}

static void afficher_resultat(const Partie *partie) {
    int noirs = compter_pions(&partie->plateau, NOIR);
    int blancs = compter_pions(&partie->plateau, BLANC);
    const Joueur *jn = &partie->joueurs[0], *jb = &partie->joueurs[1];

    printf("\n%s== Fin de la partie ==%s\n", GRAS, RESET);
    printf("%s (Noir) : %d   -   %s (Blanc) : %d\n", jn->nom, noirs, jb->nom, blancs);
    if (noirs == blancs) {
        printf("%sÉgalité !%s\n\n", JAUNE, RESET);
    } else {
        const Joueur *g = noirs > blancs ? jn : jb;
        printf("%s%s gagne !%s\n\n", g->type == HUMAIN ? VERT : ROUGE, g->nom, RESET);
    }
}

/* Demande son coup au joueur humain. Renvoie 0 s'il quitte la partie */
static int tour_humain(Partie *partie, Coup *coup) {
    char ligne[128];
    Coup coups[NB_CASES];
    char texte[4];
    int i, nb;
    long debut = millisecondes();

    while (1) {
        printf("%s, votre coup (ex. D3) : ", joueur_courant(partie)->nom);
        fflush(stdout);
        if (!lire_ligne(ligne, sizeof(ligne))) {
            return 0;
        }
        if (strcmp(ligne, "quitter") == 0 || strcmp(ligne, "q") == 0) {
            return 0;
        }
        if (strcmp(ligne, "sauver") == 0) {
            if (sauvegarder_partie(partie, FICHIER_SAUVEGARDE) == 0) {
                printf("%sPartie sauvegardée.%s\n", VERT, RESET);
            } else {
                printf("%sImpossible de sauvegarder la partie.%s\n", ROUGE, RESET);
            }
            continue;
        }
        if (strcmp(ligne, "aide") == 0 || strcmp(ligne, "?") == 0) {
            nb = coups_possibles(&partie->plateau, partie->trait, coups);
            printf("Coups possibles :");
            for (i = 0; i < nb; i++) {
                coup_vers_texte(coups[i], texte);
                printf(" %s", texte);
            }
            printf("\n");
            continue;
        }
        if (!texte_vers_coup(ligne, coup)) {
            printf("%sSaisie invalide :%s tapez une lettre de A à H et un chiffre de 1 à 8.\n", ROUGE, RESET);
            continue;
        }
        if (!coup_valide(&partie->plateau, partie->trait, coup->lig, coup->col)) {
            coup_vers_texte(*coup, texte);
            if (phase_depart(&partie->plateau)) {
                printf("%sCoup interdit en %s :%s les 4 premiers pions se posent sur D4, E4, D5 ou E5.\n",
                       ROUGE, texte, RESET);
            } else {
                printf("%sCoup interdit en %s :%s il faut encadrer au moins un pion adverse (tapez 'aide').\n",
                       ROUGE, texte, RESET);
            }
            continue;
        }
        joueur_courant(partie)->temps_ms += millisecondes() - debut;
        return 1;
    }
}

static void tour_ordinateur(Partie *partie, Coup *coup) {
    Joueur *j = joueur_courant(partie);
    Statistiques stats;
    long debut = millisecondes();

    printf("%s réfléchit...\n", j->nom);
    fflush(stdout);
    *coup = coup_ordinateur(&partie->plateau, partie->trait, j->niveau, partie->profondeur, &stats);
    j->temps_ms += millisecondes() - debut;
}

/* Boucle de jeu */
static void jouer(Partie *partie) {
    Coup coup;
    char texte[4];
    int retournes;
    Joueur *j;

    while (!partie_finie(partie)) {
        j = joueur_courant(partie);
        afficher_plateau(partie, j->type == HUMAIN);
        printf("\nAu tour de %s%s%s (%s)\n", GRAS, j->nom, RESET, nom_couleur(partie->trait));

        if (j->type == HUMAIN) {
            if (!tour_humain(partie, &coup)) {
                printf("Partie interrompue. Tapez 'sauver' avant de quitter pour la reprendre plus tard.\n");
                return;
            }
        } else {
            tour_ordinateur(partie, &coup);
        }
        retournes = partie_jouer(partie, coup);
        coup_vers_texte(coup, texte);
        printf("%s%s joue en %s%s", j->type == HUMAIN ? VERT : CYAN, j->nom, texte, RESET);
        if (retournes > 0) {
            printf(" et retourne %d pion(s)", retournes);
        }
        printf(".\n");
        if (partie->a_passe && !partie_finie(partie)) {
            printf("%s%s ne peut pas jouer et passe son tour.%s\n", JAUNE,
                   partie->joueurs[indice_joueur(adversaire(partie->trait))].nom, RESET);
        }
    }
    afficher_plateau(partie, 0);
    afficher_resultat(partie);
    remove(FICHIER_SAUVEGARDE);
}

static int demander_oui_non(const char *question) {
    char ligne[32];
    while (1) {
        printf("%s (o/n) : ", question);
        fflush(stdout);
        if (!lire_ligne(ligne, sizeof(ligne))) {
            return 0;
        }
        if (tolower((unsigned char)ligne[0]) == 'o') return 1;
        if (tolower((unsigned char)ligne[0]) == 'n') return 0;
    }
}

static int demander_niveau(const char *question, int defaut) {
    char ligne[32];
    int n;
    printf("%s (1 à 6, Entrée = %d) : ", question, defaut);
    fflush(stdout);
    if (!lire_ligne(ligne, sizeof(ligne)) || ligne[0] == '\0') {
        return defaut;
    }
    n = atoi(ligne);
    return (n >= NIVEAU_MIN && n <= NIVEAU_MAX) ? n : defaut;
}

static Joueur creer_joueur(int type, int niveau, const char *nom) {
    Joueur j;
    j.type = type;
    j.niveau = niveau;
    j.avatar = 0;
    j.temps_ms = 0;
    strncpy(j.nom, nom, LONGUEUR_NOM - 1);
    j.nom[LONGUEUR_NOM - 1] = '\0';
    return j;
}

int lancer_console(int niveau, int profondeur, int depart, int couleurs) {
    char ligne[64];
    Partie partie;
    Joueur humain, ordi, autre;

    couleurs_actives = couleurs && isatty(fileno(stdout)) && getenv("NO_COLOR") == NULL;
    printf("\n%s=== REVERSI ===%s   IA niveau %d (%s)\n", GRAS, RESET, niveau, nom_niveau(niveau));

    while (1) {
        printf("\n1 - Règles du jeu\n2 - Jouer contre l'ordinateur\n3 - Ordinateur contre ordinateur\n");
        printf("4 - Reprendre la partie sauvegardée\n5 - Quitter\nVotre choix : ");
        fflush(stdout);
        if (!lire_ligne(ligne, sizeof(ligne))) {
            return 0;
        }
        switch (atoi(ligne)) {
        case 1:
            afficher_regles();
            break;
        case 2:
            printf("Votre nom (Entrée = Joueur) : ");
            fflush(stdout);
            if (!lire_ligne(ligne, sizeof(ligne))) {
                return 0;
            }
            humain = creer_joueur(HUMAIN, 1, ligne[0] ? ligne : "Joueur");
            ordi = creer_joueur(ORDINATEUR, niveau, "Ordinateur");
            if (demander_oui_non("Voulez-vous jouer les Noirs et commencer ?")) {
                partie_initialiser(&partie, depart, &humain, &ordi, profondeur);
            } else {
                partie_initialiser(&partie, depart, &ordi, &humain, profondeur);
            }
            jouer(&partie);
            break;
        case 3:
            ordi = creer_joueur(ORDINATEUR, demander_niveau("Niveau de l'IA Noire", niveau), "Tor");
            autre = creer_joueur(ORDINATEUR, demander_niveau("Niveau de l'IA Blanche", niveau), "Zet");
            partie_initialiser(&partie, depart, &ordi, &autre, profondeur);
            jouer(&partie);
            break;
        case 4:
            if (charger_partie(&partie, FICHIER_SAUVEGARDE) != 0) {
                printf("%sAucune partie sauvegardée.%s\n", ROUGE, RESET);
            } else {
                jouer(&partie);
            }
            break;
        case 5:
            return 0;
        default:
            printf("Choix invalide.\n");
        }
    }
}
