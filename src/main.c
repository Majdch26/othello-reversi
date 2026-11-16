#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "console.h"
#include "interface.h"
#include "ia.h"
#include "plateau.h"

static void aide(const char *programme) {
    printf("Utilisation : %s [options]\n\n", programme);
    printf("  -mode Interface | Console   interface graphique (par défaut) ou terminal\n"
           "  -console                    raccourci pour -mode Console\n"
           "  -niveau N                   niveau de l'IA, de 1 à 6 (par défaut : 4)\n");
    printf("                                1 hasard, 2 arbre de profondeur 2, 3 arbre de profondeur -prof,\n"
           "                                4 alpha-bêta, 5 alpha-bêta sur plateau unique, 6 expert\n");
    printf("  -prof P                     profondeur de recherche des niveaux 3 à 6 (1 à %d)\n", PROFONDEUR_MAX);
    printf("  -depart reversi | othello   pose libre des 4 premiers pions (par défaut) ou départ classique\n"
           "  -sans-couleur               terminal sans couleurs\n");
}

/* SDL demande la signature complète de main */
int main(int argc, char *argv[]) {
    int i, console = 0, niveau = 4, profondeur = 0, depart = DEPART_REVERSI, couleurs = 1;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-mode") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "Console") == 0 || strcmp(argv[i], "console") == 0) {
                console = 1;
            } else if (strcmp(argv[i], "Interface") == 0 || strcmp(argv[i], "interface") == 0) {
                console = 0;
            } else {
                fprintf(stderr, "Mode inconnu : %s (Interface ou Console)\n", argv[i]);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-console") == 0) {
            console = 1;
        } else if (strcmp(argv[i], "-niveau") == 0 && i + 1 < argc) {
            niveau = atoi(argv[++i]);
            if (niveau < NIVEAU_MIN || niveau > NIVEAU_MAX) {
                fprintf(stderr, "Le niveau doit être compris entre %d et %d.\n", NIVEAU_MIN, NIVEAU_MAX);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-prof") == 0 && i + 1 < argc) {
            profondeur = atoi(argv[++i]);
            if (profondeur < 1 || profondeur > PROFONDEUR_MAX) {
                fprintf(stderr, "La profondeur doit être comprise entre 1 et %d.\n", PROFONDEUR_MAX);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-depart") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "othello") == 0) {
                depart = DEPART_OTHELLO;
            } else if (strcmp(argv[i], "reversi") == 0) {
                depart = DEPART_REVERSI;
            } else {
                fprintf(stderr, "Départ inconnu : %s (reversi ou othello)\n", argv[i]);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-sans-couleur") == 0) {
            couleurs = 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-aide") == 0 || strcmp(argv[i], "--help") == 0) {
            aide(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Option inconnue : %s\n\n", argv[i]);
            aide(argv[0]);
            return EXIT_FAILURE;
        }
    }

    srand((unsigned int)time(NULL));
    if (console) {
        return lancer_console(niveau, profondeur, depart, couleurs);
    }
    return lancer_interface(niveau, profondeur, depart);
}
