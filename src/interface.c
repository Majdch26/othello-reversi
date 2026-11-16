#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "interface.h"
#include "graphique.h"
#include "partie.h"
#include "ia.h"

/* ------------------------------------------------------------------------- */
/* Dimensions                                                                 */
/* ------------------------------------------------------------------------- */

#define LARGEUR 1000
#define HAUTEUR 720
#define CASE 64
#define PLATEAU_X ((LARGEUR - TAILLE * CASE) / 2)
#define PLATEAU_Y 132
#define CADRE 26
#define PION 54
#define PANNEAU_L 180
#define PANNEAU_G_X 22
#define PANNEAU_D_X (LARGEUR - PANNEAU_L - 22)
#define PANNEAU_Y PLATEAU_Y
#define PANNEAU_H 400

#define DUREE_RETOURNEMENT 320   /* Animation des pions retournés (ms) */
#define DELAI_MIN_IA 450         /* Temps minimal d'un coup de l'IA, pour qu'on le voie */
#define DELAI_IA_CONTRE_IA 650

#define OR        rgba(222, 184, 80, 255)
#define OR_PALE   rgba(222, 184, 80, 90)
#define BLANC_TXT rgba(245, 245, 235, 255)
#define GRIS_TXT  rgba(190, 205, 190, 255)
#define ROUGE_C   rgba(220, 70, 60, 255)
#define VERT_C    rgba(70, 190, 100, 255)
#define FOND_PANNEAU rgba(0, 0, 0, 95)

/* ------------------------------------------------------------------------- */
/* Ressources                                                                 */
/* ------------------------------------------------------------------------- */

typedef struct {
    Police *petite, *normale, *grande, *titre, *enorme;
    Image *logo, *bois, *tapis, *pion_noir, *pion_blanc, *regles;
    Image *gagne, *perdu, *egalite, *tor_gagne, *zet_gagne;
    Image *icone_nouvelle, *icone_sauver, *icone_regles, *icone_menu;
    Image *avatars[3], *robots[2];
} Ressources;

static Ressources R;

static int charger_ressources(void) {
    int i;
    char chemin[64];
    R.petite = gfx_charger_police("assets/polices/roboto.ttf", 14);
    R.normale = gfx_charger_police("assets/polices/roboto.ttf", 18);
    R.grande = gfx_charger_police("assets/polices/roboto.ttf", 26);
    R.titre = gfx_charger_police("assets/polices/arya.ttf", 40);
    R.enorme = gfx_charger_police("assets/polices/roboto.ttf", 40);
    R.logo = gfx_charger_image("assets/images/logo.png");
    R.bois = gfx_charger_image("assets/images/bois.jpg");
    R.tapis = gfx_charger_image("assets/images/tapis.jpg");
    R.pion_noir = gfx_charger_image("assets/images/pion_noir.png");
    R.pion_blanc = gfx_charger_image("assets/images/pion_blanc.png");
    R.regles = gfx_charger_image("assets/images/regles.png");
    R.gagne = gfx_charger_image("assets/images/gagne.png");
    R.perdu = gfx_charger_image("assets/images/perdu.png");
    R.egalite = gfx_charger_image("assets/images/egalite.png");
    R.tor_gagne = gfx_charger_image("assets/images/tor_gagne.png");
    R.zet_gagne = gfx_charger_image("assets/images/zet_gagne.png");
    R.icone_nouvelle = gfx_charger_image("assets/images/icone_nouvelle.png");
    R.icone_sauver = gfx_charger_image("assets/images/icone_sauvegarder.png");
    R.icone_regles = gfx_charger_image("assets/images/icone_regle.png");
    R.icone_menu = gfx_charger_image("assets/images/icone_quitter.png");
    for (i = 0; i < 3; i++) {
        sprintf(chemin, "assets/images/avatar%d.png", i + 1);
        R.avatars[i] = gfx_charger_image(chemin);
    }
    R.robots[0] = gfx_charger_image("assets/images/robot1.png");
    R.robots[1] = gfx_charger_image("assets/images/robot2.png");

    if (!R.petite || !R.normale || !R.grande || !R.titre || !R.enorme || !R.logo || !R.bois
        || !R.tapis || !R.pion_noir || !R.pion_blanc || !R.regles || !R.gagne || !R.perdu
        || !R.egalite || !R.tor_gagne || !R.zet_gagne || !R.icone_nouvelle || !R.icone_sauver
        || !R.icone_regles || !R.icone_menu || !R.avatars[0] || !R.avatars[1] || !R.avatars[2]
        || !R.robots[0] || !R.robots[1]) {
        fprintf(stderr, "Erreur : fichiers du dossier assets/ introuvables (%s).\n"
                        "Lancez le jeu depuis la racine du projet.\n", SDL_GetError());
        return -1;
    }
    return 0;
}

static void liberer_ressources(void) {
    int i;
    Police *polices[5];
    Image *images[25];
    polices[0] = R.petite; polices[1] = R.normale; polices[2] = R.grande;
    polices[3] = R.titre; polices[4] = R.enorme;
    images[0] = R.logo; images[1] = R.bois; images[2] = R.tapis; images[3] = R.pion_noir;
    images[4] = R.pion_blanc; images[5] = R.regles; images[6] = R.gagne; images[7] = R.perdu;
    images[8] = R.egalite; images[9] = R.tor_gagne; images[10] = R.zet_gagne;
    images[11] = R.icone_nouvelle; images[12] = R.icone_sauver; images[13] = R.icone_regles;
    images[14] = R.icone_menu; images[15] = R.avatars[0]; images[16] = R.avatars[1];
    images[17] = R.avatars[2]; images[18] = R.robots[0]; images[19] = R.robots[1];
    for (i = 0; i < 5; i++) gfx_liberer_police(polices[i]);
    for (i = 0; i < 20; i++) gfx_liberer_image(images[i]);
}

/* ------------------------------------------------------------------------- */
/* Outils de dessin                                                           */
/* ------------------------------------------------------------------------- */

typedef struct {
    int x, y, l, h;
    const char *texte;
} Bouton;

static Bouton bouton(int x, int y, int l, int h, const char *texte) {
    Bouton b;
    b.x = x; b.y = y; b.l = l; b.h = h; b.texte = texte;
    return b;
}

static int dans(const Bouton *b, int x, int y) {
    return x >= b->x && x < b->x + b->l && y >= b->y && y < b->y + b->h;
}

static void texte_centre(int xc, int y, const char *t, Police *p, Couleur c) {
    int l, h;
    gfx_taille_texte(t, p, &l, &h);
    gfx_texte(xc - l / 2, y, t, p, c);
}

static void fond(void) {
    int y;
    /* Dégradé vert, comme dans la version d'origine */
    for (y = 0; y < HAUTEUR; y += 4) {
        gfx_rect_plein(0, y, LARGEUR, 4, rgba(6, 40 + y * 60 / HAUTEUR, 18 + y * 20 / HAUTEUR, 255));
    }
}

static void panneau(int x, int y, int l, int h, int surligne) {
    gfx_rect_plein(x, y, l, h, FOND_PANNEAU);
    gfx_rect(x, y, l, h, surligne ? OR : OR_PALE);
    if (surligne) {
        gfx_rect(x + 1, y + 1, l - 2, h - 2, OR);
        gfx_rect(x + 2, y + 2, l - 4, h - 4, OR_PALE);
    }
}

/* Bouton : actif = sélectionné (option choisie) */
static void dessiner_bouton(const Bouton *b, int actif, int sx, int sy) {
    int survol = dans(b, sx, sy);
    int l, h;
    Couleur fond_b = actif ? rgba(222, 184, 80, 235) : (survol ? rgba(40, 110, 60, 240) : rgba(15, 70, 35, 230));
    gfx_rect_plein(b->x + 3, b->y + 4, b->l, b->h, rgba(0, 0, 0, 90));
    gfx_rect_plein(b->x, b->y, b->l, b->h, fond_b);
    gfx_rect(b->x, b->y, b->l, b->h, actif || survol ? OR : OR_PALE);
    gfx_taille_texte(b->texte, R.normale, &l, &h);
    gfx_texte(b->x + (b->l - l) / 2, b->y + (b->h - h) / 2, b->texte, R.normale,
              actif ? rgba(25, 30, 20, 255) : BLANC_TXT);
}

static int case_x(int col) { return PLATEAU_X + col * CASE; }
static int case_y(int lig) { return PLATEAU_Y + lig * CASE; }

/* Dessine un pion. echelle_x (0 à 100) aplatit le pion pour l'animation */
static void dessiner_pion(int lig, int col, char couleur, int echelle_x, int opacite) {
    int l = PION * echelle_x / 100;
    int x = case_x(col) + (CASE - l) / 2, y = case_y(lig) + (CASE - PION) / 2;
    Image *img = couleur == NOIR ? R.pion_noir : R.pion_blanc;
    if (l < 2) {
        l = 2;
        x = case_x(col) + CASE / 2 - 1;
    }
    gfx_disque(case_x(col) + CASE / 2 + 2, case_y(lig) + CASE / 2 + 3, PION / 2 * echelle_x / 100, rgba(0, 0, 0, 70 * opacite / 255));
    gfx_image_transparente(img, x, y, l, PION, opacite);
}

/* ------------------------------------------------------------------------- */
/* Plateau et panneaux                                                        */
/* ------------------------------------------------------------------------- */

typedef struct {
    Plateau avant;          /* Position avant le dernier coup (pour l'animation) */
    unsigned int debut;     /* Début de l'animation, 0 si aucune */
} Animation;

static void dessiner_plateau(const Partie *partie, const Animation *anim, int montrer_coups,
                             int sx, int sy, int case_erreur_lig, int case_erreur_col) {
    int i, j, k;
    char lettre[2] = "A", chiffre[2] = "1";
    const Plateau *p = &partie->plateau;
    unsigned int t = gfx_temps();
    int progression = 100;

    if (anim->debut != 0 && t - anim->debut < DUREE_RETOURNEMENT) {
        progression = (int)((t - anim->debut) * 100 / DUREE_RETOURNEMENT);
    }

    /* Cadre en bois et tapis vert */
    gfx_rect_plein(PLATEAU_X - CADRE + 6, PLATEAU_Y - CADRE + 8, TAILLE * CASE + 2 * CADRE,
                   TAILLE * CASE + 2 * CADRE, rgba(0, 0, 0, 110));
    gfx_image_remplir(R.bois, PLATEAU_X - CADRE, PLATEAU_Y - CADRE, TAILLE * CASE + 2 * CADRE,
                      TAILLE * CASE + 2 * CADRE);
    gfx_image_remplir(R.tapis, PLATEAU_X, PLATEAU_Y, TAILLE * CASE, TAILLE * CASE);
    gfx_rect_plein(PLATEAU_X, PLATEAU_Y, TAILLE * CASE, TAILLE * CASE, rgba(0, 60, 20, 60));
    for (k = 0; k <= TAILLE; k++) {
        gfx_rect_plein(PLATEAU_X + k * CASE - 1, PLATEAU_Y, 2, TAILLE * CASE, rgba(0, 30, 10, 170));
        gfx_rect_plein(PLATEAU_X, PLATEAU_Y + k * CASE - 1, TAILLE * CASE, 2, rgba(0, 30, 10, 170));
    }
    /* Petits points de repère, comme sur un vrai othellier */
    gfx_disque(PLATEAU_X + 2 * CASE, PLATEAU_Y + 2 * CASE, 4, rgba(0, 30, 10, 200));
    gfx_disque(PLATEAU_X + 6 * CASE, PLATEAU_Y + 2 * CASE, 4, rgba(0, 30, 10, 200));
    gfx_disque(PLATEAU_X + 2 * CASE, PLATEAU_Y + 6 * CASE, 4, rgba(0, 30, 10, 200));
    gfx_disque(PLATEAU_X + 6 * CASE, PLATEAU_Y + 6 * CASE, 4, rgba(0, 30, 10, 200));

    /* Coordonnées */
    for (k = 0; k < TAILLE; k++) {
        lettre[0] = (char)('A' + k);
        chiffre[0] = (char)('1' + k);
        texte_centre(case_x(k) + CASE / 2, PLATEAU_Y - CADRE + 3, lettre, R.petite, rgba(255, 235, 200, 230));
        texte_centre(PLATEAU_X - CADRE / 2, case_y(k) + CASE / 2 - 9, chiffre, R.petite, rgba(255, 235, 200, 230));
    }

    for (i = 0; i < TAILLE; i++) {
        for (j = 0; j < TAILLE; j++) {
            char c = p->cases[i][j];
            if (c == VIDE) {
                if (montrer_coups && coup_valide(p, partie->trait, i, j)) {
                    int survol = sx >= case_x(j) && sx < case_x(j) + CASE && sy >= case_y(i) && sy < case_y(i) + CASE;
                    if (survol) {
                        dessiner_pion(i, j, partie->trait, 100, 120);
                    } else {
                        gfx_disque(case_x(j) + CASE / 2, case_y(i) + CASE / 2, 7, rgba(0, 0, 0, 90));
                        gfx_cercle(case_x(j) + CASE / 2, case_y(i) + CASE / 2, 8, 2, rgba(255, 255, 255, 70));
                    }
                }
            } else if (progression < 100 && anim->avant.cases[i][j] != VIDE && anim->avant.cases[i][j] != c) {
                /* Pion en train de se retourner : il s'aplatit puis réapparaît */
                if (progression < 50) {
                    dessiner_pion(i, j, anim->avant.cases[i][j], 100 - progression * 2, 255);
                } else {
                    dessiner_pion(i, j, c, (progression - 50) * 2, 255);
                }
            } else {
                int apparition = (progression < 100 && anim->avant.cases[i][j] == VIDE);
                dessiner_pion(i, j, c, 100, apparition ? 120 + progression * 135 / 100 : 255);
            }
        }
    }

    /* Dernier coup joué */
    if (partie->dernier_coup.lig >= 0) {
        gfx_disque(case_x(partie->dernier_coup.col) + CASE / 2, case_y(partie->dernier_coup.lig) + CASE / 2,
                   5, ROUGE_C);
    }
    /* Clic sur une case interdite */
    if (case_erreur_lig >= 0) {
        gfx_rect(case_x(case_erreur_col) + 2, case_y(case_erreur_lig) + 2, CASE - 4, CASE - 4, ROUGE_C);
        gfx_rect(case_x(case_erreur_col) + 3, case_y(case_erreur_lig) + 3, CASE - 6, CASE - 6, ROUGE_C);
    }
}

static Image *image_joueur(const Joueur *j, int indice) {
    if (j->type == ORDINATEUR) {
        return R.robots[indice];
    }
    return R.avatars[j->avatar >= 0 && j->avatar < 3 ? j->avatar : 0];
}

static void dessiner_joueur(const Partie *partie, int indice, int reflechit) {
    const Joueur *j = &partie->joueurs[indice];
    char couleur = indice == 0 ? NOIR : BLANC;
    int x = indice == 0 ? PANNEAU_G_X : PANNEAU_D_X, xc = x + PANNEAU_L / 2;
    int au_trait = partie->trait == couleur && !partie_finie(partie);
    char texte[64];
    unsigned int t = gfx_temps();

    panneau(x, PANNEAU_Y, PANNEAU_L, PANNEAU_H, au_trait);
    gfx_disque(xc, PANNEAU_Y + 72, 56, rgba(255, 255, 255, 30));
    gfx_image(image_joueur(j, indice), xc - 52, PANNEAU_Y + 20, 104, 104);
    texte_centre(xc, PANNEAU_Y + 134, j->nom, R.normale, BLANC_TXT);
    if (j->type == ORDINATEUR) {
        sprintf(texte, "IA niveau %d - %s", j->niveau, nom_niveau(j->niveau));
    } else {
        sprintf(texte, "Joueur humain");
    }
    texte_centre(xc, PANNEAU_Y + 158, texte, R.petite, GRIS_TXT);

    gfx_image(couleur == NOIR ? R.pion_noir : R.pion_blanc, xc - 58, PANNEAU_Y + 195, 46, 46);
    sprintf(texte, "%d", compter_pions(&partie->plateau, couleur));
    gfx_texte(xc - 2, PANNEAU_Y + 196, texte, R.enorme, BLANC_TXT);

    sprintf(texte, "Réserve : %d pion%s", partie->plateau.stock[indice], partie->plateau.stock[indice] > 1 ? "s" : "");
    texte_centre(xc, PANNEAU_Y + 262, texte, R.petite, GRIS_TXT);
    sprintf(texte, "Réflexion : %.1f s", j->temps_ms / 1000.0);
    texte_centre(xc, PANNEAU_Y + 284, texte, R.petite, GRIS_TXT);

    if (au_trait) {
        if (reflechit || j->type == ORDINATEUR) {
            /* Trois points animés */
            int k, phase = (int)(t / 250) % 4;
            for (k = 0; k < 3; k++) {
                gfx_disque(xc - 16 + k * 16, PANNEAU_Y + 352, 5, k < phase ? OR : OR_PALE);
            }
            texte_centre(xc, PANNEAU_Y + 318, "réfléchit", R.normale, OR);
        } else {
            texte_centre(xc, PANNEAU_Y + 330, "à vous de jouer", R.normale, OR);
        }
    }
}

/* Barre d'icônes en haut à droite */
#define NB_ICONES 4
static Bouton icones[NB_ICONES];
static const char *LEGENDES[NB_ICONES] = {"Nouvelle", "Sauver", "Règles", "Menu"};

static void placer_icones(void) {
    int k;
    for (k = 0; k < NB_ICONES; k++) {
        icones[k] = bouton(LARGEUR - 4 * 62 - 10 + k * 62, 14, 50, 50, LEGENDES[k]);
    }
}

static void dessiner_icones(int sx, int sy, int sauvegarde_possible) {
    Image *img[NB_ICONES];
    int k, opacite;
    img[0] = R.icone_nouvelle; img[1] = R.icone_sauver; img[2] = R.icone_regles; img[3] = R.icone_menu;
    for (k = 0; k < NB_ICONES; k++) {
        int s = dans(&icones[k], sx, sy) ? 4 : 0;
        opacite = (k == 1 && !sauvegarde_possible) ? 90 : 255;
        gfx_image_transparente(img[k], icones[k].x - s / 2, icones[k].y - s / 2, 50 + s, 50 + s, opacite);
        texte_centre(icones[k].x + 25, icones[k].y + 54, LEGENDES[k], R.petite, s ? OR : GRIS_TXT);
    }
}

static void dessiner_entete(void) {
    gfx_image(R.logo, 30, 6, 250, 100);
}

static void dessiner_message(const char *message, Couleur c) {
    if (message != NULL && message[0] != '\0') {
        int l, h;
        gfx_taille_texte(message, R.normale, &l, &h);
        gfx_rect_plein(LARGEUR / 2 - l / 2 - 18, HAUTEUR - 46, l + 36, 34, rgba(0, 0, 0, 120));
        gfx_texte(LARGEUR / 2 - l / 2, HAUTEUR - 46 + (34 - h) / 2, message, R.normale, c);
    }
}

/* ------------------------------------------------------------------------- */
/* IA dans un thread : la fenêtre reste réactive pendant la réflexion         */
/* ------------------------------------------------------------------------- */

typedef struct {
    Plateau plateau;
    char trait;
    int niveau;
    int profondeur;
    Coup resultat;
    unsigned int duree;         /* Temps de calcul réel, en ms */
    SDL_atomic_t fini;
} TacheIA;

static int executer_ia(void *donnees) {
    TacheIA *t = (TacheIA *)donnees;
    unsigned int debut = gfx_temps();
    t->resultat = coup_ordinateur(&t->plateau, t->trait, t->niveau, t->profondeur, NULL);
    t->duree = gfx_temps() - debut;
    SDL_AtomicSet(&t->fini, 1);
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Écrans                                                                     */
/* ------------------------------------------------------------------------- */

enum { MENU_JOUER, MENU_IA, MENU_CONTINUER, MENU_REGLES, MENU_QUITTER };

static void ecran_regles(void) {
    Bouton retour = bouton(LARGEUR / 2 - 90, HAUTEUR - 62, 180, 44, "Retour");
    Evenement ev;
    int sx, sy;
    while (1) {
        gfx_souris(&sx, &sy);
        fond();
        gfx_image(R.regles, 60, 20, 380, 570);
        panneau(480, 40, 470, 560, 0);
        gfx_texte(505, 60, "Règles du Reversi", R.titre, OR);
        gfx_texte(505, 130, "• Les Noirs commencent.", R.normale, BLANC_TXT);
        gfx_texte(505, 165, "• Départ Reversi : les 4 premiers pions", R.normale, BLANC_TXT);
        gfx_texte(505, 190, "  se posent au choix sur D4, E4, D5, E5.", R.normale, BLANC_TXT);
        gfx_texte(505, 215, "  (Départ Othello : ils sont déjà placés.)", R.normale, GRIS_TXT);
        gfx_texte(505, 250, "• Ensuite, chaque pion doit encadrer au", R.normale, BLANC_TXT);
        gfx_texte(505, 275, "  moins un pion adverse, dans l'une des", R.normale, BLANC_TXT);
        gfx_texte(505, 300, "  8 directions : ces pions sont retournés.", R.normale, BLANC_TXT);
        gfx_texte(505, 335, "• Si vous ne pouvez pas jouer, vous", R.normale, BLANC_TXT);
        gfx_texte(505, 360, "  passez votre tour.", R.normale, BLANC_TXT);
        gfx_texte(505, 395, "• Chaque joueur a 32 pions ; si sa réserve", R.normale, BLANC_TXT);
        gfx_texte(505, 420, "  est vide, l'adversaire lui en donne.", R.normale, BLANC_TXT);
        gfx_texte(505, 455, "• La partie finit quand plus personne ne", R.normale, BLANC_TXT);
        gfx_texte(505, 480, "  peut jouer : le plus de pions gagne.", R.normale, BLANC_TXT);
        gfx_texte(505, 525, "Les cases jouables sont marquées d'un point.", R.petite, GRIS_TXT);
        dessiner_bouton(&retour, 0, sx, sy);
        gfx_actualiser();
        if (gfx_attendre_evenement(&ev, 30)) {
            if ((ev.type == EV_CLIC && dans(&retour, ev.x, ev.y))
                || (ev.type == EV_TOUCHE && (ev.touche == SDLK_ESCAPE || ev.touche == SDLK_RETURN))) {
                return;
            }
        }
    }
}

static int menu_principal(void) {
    Bouton b[5];
    const int choix[5] = {MENU_JOUER, MENU_IA, MENU_CONTINUER, MENU_REGLES, MENU_QUITTER};
    int k, sx, sy, sauvegarde = sauvegarde_existe(FICHIER_SAUVEGARDE);
    Evenement ev;

    b[0] = bouton(LARGEUR / 2 - 160, 300, 320, 54, "Vous contre l'IA");
    b[1] = bouton(LARGEUR / 2 - 160, 368, 320, 54, "IA contre IA");
    b[2] = bouton(LARGEUR / 2 - 160, 436, 320, 54, "Continuer la partie");
    b[3] = bouton(LARGEUR / 2 - 160, 504, 320, 54, "Règles du jeu");
    b[4] = bouton(LARGEUR / 2 - 160, 572, 320, 54, "Quitter");

    while (1) {
        sauvegarde = sauvegarde_existe(FICHIER_SAUVEGARDE);
        gfx_souris(&sx, &sy);
        fond();
        gfx_image(R.logo, LARGEUR / 2 - 250, 40, 500, 200);
        gfx_image(R.pion_noir, LARGEUR / 2 - 300, 400, 90, 90);
        gfx_image(R.pion_blanc, LARGEUR / 2 + 210, 400, 90, 90);
        for (k = 0; k < 5; k++) {
            if (k == 2 && !sauvegarde) {
                gfx_rect_plein(b[k].x, b[k].y, b[k].l, b[k].h, rgba(0, 0, 0, 60));
                gfx_rect(b[k].x, b[k].y, b[k].l, b[k].h, rgba(255, 255, 255, 30));
                texte_centre(LARGEUR / 2, b[k].y + 16, "Aucune partie sauvegardée", R.normale, rgba(255, 255, 255, 80));
            } else {
                dessiner_bouton(&b[k], 0, sx, sy);
            }
        }
        texte_centre(LARGEUR / 2, HAUTEUR - 34, "Projet de programmation impérative - Licence Informatique, Université Jean Monnet",
                     R.petite, rgba(255, 255, 255, 110));
        gfx_actualiser();

        if (gfx_attendre_evenement(&ev, 30)) {
            if (ev.type == EV_CLIC) {
                for (k = 0; k < 5; k++) {
                    if (dans(&b[k], ev.x, ev.y) && (k != 2 || sauvegarde)) {
                        return choix[k];
                    }
                }
            } else if (ev.type == EV_TOUCHE && ev.touche == SDLK_ESCAPE) {
                return MENU_QUITTER;
            }
        }
    }
}

/* Configuration d'une partie. contre_ia = 0 : humain contre IA ; 1 : IA contre IA.
   Renvoie 0 si le joueur revient au menu */
static int ecran_configuration(int contre_ia, Partie *partie, int *niveau_defaut, int profondeur, int *depart) {
    static char nom[LONGUEUR_NOM] = "";
    static int avatar = 0, noirs = 1;
    int niveau_ia2 = *niveau_defaut;
    Bouton b_noir, b_blanc, b_niveaux[6], b_niveaux2[6], b_reversi, b_othello, commencer, retour, b_avatars[3], champ;
    Evenement ev;
    int k, sx, sy, y0 = 140;
    char texte[64];
    Joueur j1, j2;

    for (k = 0; k < 6; k++) {
        b_niveaux[k] = bouton(370 + k * 92, 0, 84, 40, "");
        b_niveaux2[k] = bouton(370 + k * 92, 0, 84, 40, "");
    }
    champ = bouton(370, y0, 360, 44, "");
    for (k = 0; k < 3; k++) {
        b_avatars[k] = bouton(370 + k * 90, y0 + 64, 76, 76, "");
    }
    b_noir = bouton(370, y0 + 166, 260, 48, "Noirs (je commence)");
    b_blanc = bouton(640, y0 + 166, 260, 48, "Blancs (l'IA commence)");
    for (k = 0; k < 6; k++) {
        b_niveaux[k].y = contre_ia ? y0 : y0 + 240;
        b_niveaux2[k].y = y0 + 90;
    }
    b_reversi = bouton(370, contre_ia ? y0 + 180 : y0 + 330, 260, 44, "Reversi (pose libre)");
    b_othello = bouton(640, contre_ia ? y0 + 180 : y0 + 330, 260, 44, "Othello (4 pions placés)");
    commencer = bouton(LARGEUR / 2 + 20, HAUTEUR - 90, 220, 52, "Commencer");
    retour = bouton(LARGEUR / 2 - 240, HAUTEUR - 90, 220, 52, "Retour");

    if (!contre_ia) {
        gfx_saisie_texte(1);
    }
    while (1) {
        gfx_souris(&sx, &sy);
        fond();
        dessiner_entete();
        texte_centre(LARGEUR / 2, 70, contre_ia ? "IA contre IA" : "Nouvelle partie", R.titre, OR);

        if (!contre_ia) {
            gfx_texte(110, y0 + 10, "Votre nom", R.grande, BLANC_TXT);
            gfx_rect_plein(champ.x, champ.y, champ.l, champ.h, rgba(255, 255, 255, 235));
            gfx_rect(champ.x, champ.y, champ.l, champ.h, OR);
            gfx_texte(champ.x + 12, champ.y + 10, nom[0] ? nom : "", R.normale, rgba(20, 30, 20, 255));
            if ((gfx_temps() / 500) % 2 == 0) {
                int l, h;
                gfx_taille_texte(nom[0] ? nom : " ", R.normale, &l, &h);
                gfx_rect_plein(champ.x + 14 + (nom[0] ? l : 0), champ.y + 10, 2, 24, rgba(20, 30, 20, 255));
            }
            gfx_texte(110, y0 + 90, "Votre avatar", R.grande, BLANC_TXT);
            for (k = 0; k < 3; k++) {
                gfx_image(R.avatars[k], b_avatars[k].x, b_avatars[k].y, 76, 76);
                if (avatar == k) {
                    gfx_cercle(b_avatars[k].x + 38, b_avatars[k].y + 38, 41, 4, OR);
                }
            }
            gfx_texte(110, y0 + 176, "Votre couleur", R.grande, BLANC_TXT);
            dessiner_bouton(&b_noir, noirs, sx, sy);
            dessiner_bouton(&b_blanc, !noirs, sx, sy);
            gfx_texte(110, y0 + 248, "Niveau de l'IA", R.grande, BLANC_TXT);
        } else {
            gfx_texte(110, y0 + 8, "Tor (Noirs)", R.grande, BLANC_TXT);
            gfx_texte(110, y0 + 98, "Zet (Blancs)", R.grande, BLANC_TXT);
        }
        for (k = 0; k < 6; k++) {
            sprintf(texte, "%d", k + 1);
            b_niveaux[k].texte = texte;
            dessiner_bouton(&b_niveaux[k], *niveau_defaut == k + 1, sx, sy);
            if (contre_ia) {
                b_niveaux2[k].texte = texte;
                dessiner_bouton(&b_niveaux2[k], niveau_ia2 == k + 1, sx, sy);
            }
        }
        sprintf(texte, "%s : %s", "Niveau", nom_niveau(*niveau_defaut));
        gfx_texte(370, b_niveaux[0].y + 48, texte, R.petite, GRIS_TXT);
        if (contre_ia) {
            sprintf(texte, "%s : %s", "Niveau", nom_niveau(niveau_ia2));
            gfx_texte(370, b_niveaux2[0].y + 48, texte, R.petite, GRIS_TXT);
        }
        gfx_texte(110, b_reversi.y + 8, "Départ", R.grande, BLANC_TXT);
        dessiner_bouton(&b_reversi, *depart == DEPART_REVERSI, sx, sy);
        dessiner_bouton(&b_othello, *depart == DEPART_OTHELLO, sx, sy);
        dessiner_bouton(&retour, 0, sx, sy);
        dessiner_bouton(&commencer, 0, sx, sy);
        gfx_actualiser();

        if (!gfx_attendre_evenement(&ev, 30)) {
            continue;
        }
        if (ev.type == EV_TEXTE && !contre_ia) {
            if (strlen(nom) + strlen(ev.texte) < LONGUEUR_NOM - 1) {
                strcat(nom, ev.texte);
            }
        } else if (ev.type == EV_TOUCHE) {
            if (ev.touche == SDLK_BACKSPACE && nom[0] != '\0') {
                /* Efface le dernier caractère UTF-8 */
                int n = (int)strlen(nom) - 1;
                while (n > 0 && ((unsigned char)nom[n] & 0xC0) == 0x80) {
                    n--;
                }
                nom[n] = '\0';
            } else if (ev.touche == SDLK_ESCAPE) {
                gfx_saisie_texte(0);
                return 0;
            } else if (ev.touche == SDLK_RETURN) {
                break;
            }
        } else if (ev.type == EV_CLIC) {
            if (dans(&retour, ev.x, ev.y)) {
                gfx_saisie_texte(0);
                return 0;
            }
            if (dans(&commencer, ev.x, ev.y)) {
                break;
            }
            if (dans(&b_noir, ev.x, ev.y)) noirs = 1;
            if (dans(&b_blanc, ev.x, ev.y)) noirs = 0;
            if (dans(&b_reversi, ev.x, ev.y)) *depart = DEPART_REVERSI;
            if (dans(&b_othello, ev.x, ev.y)) *depart = DEPART_OTHELLO;
            for (k = 0; k < 6; k++) {
                if (dans(&b_niveaux[k], ev.x, ev.y)) *niveau_defaut = k + 1;
                if (contre_ia && dans(&b_niveaux2[k], ev.x, ev.y)) niveau_ia2 = k + 1;
            }
            for (k = 0; k < 3 && !contre_ia; k++) {
                if (dans(&b_avatars[k], ev.x, ev.y)) avatar = k;
            }
        }
    }
    gfx_saisie_texte(0);

    if (contre_ia) {
        j1.type = j2.type = ORDINATEUR;
        j1.niveau = *niveau_defaut;
        j2.niveau = niveau_ia2;
        strcpy(j1.nom, "Tor");
        strcpy(j2.nom, "Zet");
        j1.avatar = j2.avatar = 0;
        partie_initialiser(partie, *depart, &j1, &j2, profondeur);
    } else {
        j1.type = HUMAIN;
        j1.niveau = 1;
        j1.avatar = avatar;
        strncpy(j1.nom, nom[0] ? nom : "Joueur", LONGUEUR_NOM - 1);
        j1.nom[LONGUEUR_NOM - 1] = '\0';
        j2.type = ORDINATEUR;
        j2.niveau = *niveau_defaut;
        j2.avatar = 0;
        strcpy(j2.nom, "Tor");
        if (noirs) {
            partie_initialiser(partie, *depart, &j1, &j2, profondeur);
        } else {
            strcpy(j2.nom, "Zet");
            partie_initialiser(partie, *depart, &j2, &j1, profondeur);
        }
    }
    return 1;
}

/* Écran de fin. Renvoie 1 pour rejouer, 0 pour le menu */
static int ecran_fin(const Partie *partie, const Animation *anim) {
    int noirs = compter_pions(&partie->plateau, NOIR), blancs = compter_pions(&partie->plateau, BLANC);
    int humain = partie->joueurs[0].type == HUMAIN ? 0 : (partie->joueurs[1].type == HUMAIN ? 1 : -1);
    Image *img;
    char texte[96];
    Bouton rejouer = bouton(LARGEUR / 2 - 200, 600, 180, 48, "Rejouer");
    Bouton menu = bouton(LARGEUR / 2 + 20, 600, 180, 48, "Menu");
    Evenement ev;
    int sx, sy;
    unsigned int debut = gfx_temps();

    if (noirs == blancs) {
        img = R.egalite;
    } else if (humain >= 0) {
        img = ((noirs > blancs) == (humain == 0)) ? R.gagne : R.perdu;
    } else {
        img = noirs > blancs ? R.tor_gagne : R.zet_gagne;
    }
    sprintf(texte, "%s %d  -  %d %s", partie->joueurs[0].nom, noirs, blancs, partie->joueurs[1].nom);

    while (1) {
        unsigned int t = gfx_temps() - debut;
        int opacite = t > 400 ? 255 : (int)(t * 255 / 400);
        gfx_souris(&sx, &sy);
        fond();
        dessiner_entete();
        dessiner_plateau(partie, anim, 0, -1, -1, -1, -1);
        dessiner_joueur(partie, 0, 0);
        dessiner_joueur(partie, 1, 0);
        gfx_rect_plein(0, 0, LARGEUR, HAUTEUR, rgba(0, 0, 0, opacite * 150 / 255));
        gfx_image_transparente(img, LARGEUR / 2 - 190, 110, 380, 380, opacite);
        gfx_rect_plein(LARGEUR / 2 - 230, 510, 460, 64, rgba(0, 0, 0, opacite * 150 / 255));
        texte_centre(LARGEUR / 2, 522, texte, R.grande, OR);
        dessiner_bouton(&rejouer, 0, sx, sy);
        dessiner_bouton(&menu, 0, sx, sy);
        gfx_actualiser();
        if (gfx_attendre_evenement(&ev, 30) && t > 300) {
            if (ev.type == EV_CLIC && dans(&rejouer, ev.x, ev.y)) return 1;
            if (ev.type == EV_CLIC && dans(&menu, ev.x, ev.y)) return 0;
            if (ev.type == EV_TOUCHE && ev.touche == SDLK_ESCAPE) return 0;
            if (ev.type == EV_TOUCHE && ev.touche == SDLK_RETURN) return 1;
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Partie                                                                     */
/* ------------------------------------------------------------------------- */

enum { FIN_MENU, FIN_REJOUER };

static int jouer_partie(Partie *partie) {
    Animation anim;
    TacheIA tache;
    SDL_Thread *thread = NULL;
    unsigned int debut_ia = 0, debut_tour = gfx_temps(), fin_message = 0, fin_erreur = 0;
    char message[128] = "";
    Couleur couleur_message = BLANC_TXT;
    int sx, sy, err_lig = -1, err_col = -1, termine = 0, ia_contre_ia;
    Evenement ev;
    Joueur *j;
    Coup coup;
    char texte[4];

    placer_icones();
    anim.avant = partie->plateau;
    anim.debut = 0;
    ia_contre_ia = partie->joueurs[0].type == ORDINATEUR && partie->joueurs[1].type == ORDINATEUR;

    while (1) {
        unsigned int t = gfx_temps();
        int animation_finie = anim.debut == 0 || t - anim.debut >= DUREE_RETOURNEMENT;
        j = joueur_courant(partie);
        termine = partie_finie(partie);

        /* --- Tour de l'ordinateur : lancement puis récupération du coup --- */
        if (!termine && j->type == ORDINATEUR && animation_finie) {
            if (thread == NULL) {
                if (t - anim.debut >= (unsigned int)(ia_contre_ia ? DELAI_IA_CONTRE_IA : 150) || anim.debut == 0) {
                    tache.plateau = partie->plateau;
                    tache.trait = partie->trait;
                    tache.niveau = j->niveau;
                    tache.profondeur = partie->profondeur;
                    SDL_AtomicSet(&tache.fini, 0);
                    debut_ia = t;
                    thread = SDL_CreateThread(executer_ia, "ia", &tache);
                    if (thread == NULL) {
                        executer_ia(&tache);
                    }
                }
            } else if (SDL_AtomicGet(&tache.fini) && t - debut_ia >= DELAI_MIN_IA) {
                SDL_WaitThread(thread, NULL);
                thread = NULL;
                j->temps_ms += tache.duree;
                coup = tache.resultat;
                anim.avant = partie->plateau;
                anim.debut = t;
                partie_jouer(partie, coup);
                coup_vers_texte(coup, texte);
                sprintf(message, "%s joue en %s", j->nom, texte);
                couleur_message = GRIS_TXT;
                fin_message = t + 2500;
                if (partie->a_passe && !partie_finie(partie)) {
                    sprintf(message, "%s ne peut pas jouer : il passe son tour",
                            partie->joueurs[indice_joueur(adversaire(partie->trait))].nom);
                    couleur_message = OR;
                    fin_message = t + 3000;
                }
                debut_tour = t;
            }
        }

        /* --- Affichage --- */
        gfx_souris(&sx, &sy);
        fond();
        dessiner_entete();
        dessiner_icones(sx, sy, !termine && thread == NULL);
        dessiner_plateau(partie, &anim, !termine && j->type == HUMAIN && animation_finie, sx, sy,
                         t < fin_erreur ? err_lig : -1, err_col);
        dessiner_joueur(partie, 0, thread != NULL && partie->trait == NOIR);
        dessiner_joueur(partie, 1, thread != NULL && partie->trait == BLANC);
        if (t < fin_message) {
            dessiner_message(message, couleur_message);
        } else if (!termine && j->type == HUMAIN && phase_depart(&partie->plateau)) {
            dessiner_message("Posez vos premiers pions sur l'une des 4 cases centrales", GRIS_TXT);
        } else if (termine) {
            dessiner_message("Partie terminée", OR);
        }
        gfx_actualiser();

        /* --- Fin de partie (après la dernière animation) --- */
        if (termine && animation_finie && thread == NULL && t - anim.debut > 700) {
            remove(FICHIER_SAUVEGARDE);
            return ecran_fin(partie, &anim) ? FIN_REJOUER : FIN_MENU;
        }

        /* --- Événements --- */
        if (!gfx_attendre_evenement(&ev, 16)) {
            continue;
        }
        if (ev.type == EV_TOUCHE && ev.touche == SDLK_ESCAPE) {
            ev.type = EV_CLIC;
            ev.x = icones[3].x + 1;
            ev.y = icones[3].y + 1;
        }
        if (ev.type != EV_CLIC) {
            continue;
        }
        if (dans(&icones[0], ev.x, ev.y) || dans(&icones[3], ev.x, ev.y)) {
            /* Nouvelle partie ou menu : on attend la fin de la réflexion en cours */
            if (thread != NULL) {
                SDL_WaitThread(thread, NULL);
            }
            return dans(&icones[0], ev.x, ev.y) ? FIN_REJOUER : FIN_MENU;
        }
        if (dans(&icones[1], ev.x, ev.y) && !termine && thread == NULL) {
            if (sauvegarder_partie(partie, FICHIER_SAUVEGARDE) == 0) {
                strcpy(message, "Partie sauvegardée : reprenez-la depuis le menu");
                couleur_message = VERT_C;
            } else {
                strcpy(message, "Impossible de sauvegarder (dossier saves/ manquant ?)");
                couleur_message = ROUGE_C;
            }
            fin_message = t + 2500;
            continue;
        }
        if (dans(&icones[2], ev.x, ev.y)) {
            if (thread != NULL) {
                SDL_WaitThread(thread, NULL);
                thread = NULL;
                debut_ia = 0;
            }
            ecran_regles();
            continue;
        }
        /* Clic sur le plateau */
        if (!termine && j->type == HUMAIN && animation_finie
            && ev.x >= PLATEAU_X && ev.x < PLATEAU_X + TAILLE * CASE
            && ev.y >= PLATEAU_Y && ev.y < PLATEAU_Y + TAILLE * CASE) {
            coup.col = (ev.x - PLATEAU_X) / CASE;
            coup.lig = (ev.y - PLATEAU_Y) / CASE;
            if (coup_valide(&partie->plateau, partie->trait, coup.lig, coup.col)) {
                j->temps_ms += t - debut_tour;
                anim.avant = partie->plateau;
                anim.debut = t;
                partie_jouer(partie, coup);
                fin_message = 0;
                if (partie->a_passe && !partie_finie(partie)) {
                    sprintf(message, "%s ne peut pas jouer : à vous de rejouer",
                            partie->joueurs[indice_joueur(adversaire(partie->trait))].nom);
                    couleur_message = OR;
                    fin_message = t + 3000;
                }
                debut_tour = t;
            } else {
                err_lig = coup.lig;
                err_col = coup.col;
                fin_erreur = t + 500;
                strcpy(message, phase_depart(&partie->plateau)
                                ? "Les 4 premiers pions se posent sur les cases centrales"
                                : "Coup interdit : il faut encadrer au moins un pion adverse");
                couleur_message = ROUGE_C;
                fin_message = t + 2000;
            }
        }
    }
}

int lancer_interface(int niveau, int profondeur, int depart) {
    Partie partie;
    int choix, resultat, contre_ia = 0;

    if (gfx_ouvrir("Othello - Reversi", LARGEUR, HAUTEUR) != 0) {
        return 1;
    }
    if (charger_ressources() != 0) {
        gfx_fermer();
        return 1;
    }

    while (1) {
        choix = menu_principal();
        if (choix == MENU_QUITTER) {
            break;
        }
        if (choix == MENU_REGLES) {
            ecran_regles();
            continue;
        }
        if (choix == MENU_CONTINUER) {
            if (charger_partie(&partie, FICHIER_SAUVEGARDE) != 0) {
                continue;
            }
            resultat = jouer_partie(&partie);
            contre_ia = partie.joueurs[0].type == ORDINATEUR && partie.joueurs[1].type == ORDINATEUR;
        } else {
            contre_ia = (choix == MENU_IA);
            if (!ecran_configuration(contre_ia, &partie, &niveau, profondeur, &depart)) {
                continue;
            }
            resultat = jouer_partie(&partie);
        }
        /* Rejouer : même configuration, nouvelle partie */
        while (resultat == FIN_REJOUER) {
            Joueur j1 = partie.joueurs[0], j2 = partie.joueurs[1];
            partie_initialiser(&partie, depart, &j1, &j2, profondeur);
            resultat = jouer_partie(&partie);
        }
        (void)contre_ia;
    }

    liberer_ressources();
    gfx_fermer();
    return 0;
}
