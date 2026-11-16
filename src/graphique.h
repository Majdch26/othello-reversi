#ifndef GRAPHIQUE_H
#define GRAPHIQUE_H

/* Petite couche graphique au-dessus de SDL2 : fenêtre, rectangles, texte,
   images, temps et événements. Le reste du jeu n'utilise que ces fonctions. */

#include <SDL.h>
#include <SDL_ttf.h>

/* Couleur codée sur 32 bits : 0xRRGGBBAA */
typedef Uint32 Couleur;

typedef TTF_Font Police;

typedef struct {
    SDL_Texture *texture;
    int largeur;
    int hauteur;
} Image;

/* Événement simplifié */
enum { EV_AUCUN, EV_TOUCHE, EV_CLIC, EV_TEXTE };

typedef struct {
    int type;            /* EV_AUCUN, EV_TOUCHE ou EV_CLIC */
    SDL_Keycode touche;  /* Touche appuyée (EV_TOUCHE) */
    int x, y;            /* Position du clic gauche (EV_CLIC) */
    char texte[32];      /* Caractères tapés (EV_TEXTE), en UTF-8 */
} Evenement;

Couleur rgba(int r, int v, int b, int a);
void composantes(Couleur c, int *r, int *v, int *b, int *a);

/* Fenêtre. Renvoie 0 si succès, -1 sinon */
int gfx_ouvrir(const char *titre, int largeur, int hauteur);
void gfx_fermer(void);
void gfx_actualiser(void);

/* Dessin (la transparence est prise en compte) */
void gfx_rect_plein(int x, int y, int largeur, int hauteur, Couleur c);
void gfx_rect(int x, int y, int largeur, int hauteur, Couleur c);
void gfx_disque(int cx, int cy, int rayon, Couleur c);
void gfx_cercle(int cx, int cy, int rayon, int epaisseur, Couleur c);

/* Texte (UTF-8) */
Police *gfx_charger_police(const char *chemin, int taille);
void gfx_liberer_police(Police *police);
void gfx_texte(int x, int y, const char *texte, Police *police, Couleur c);
void gfx_taille_texte(const char *texte, Police *police, int *largeur, int *hauteur);

/* Images. largeur ou hauteur <= 0 : taille d'origine */
Image *gfx_charger_image(const char *chemin);
void gfx_liberer_image(Image *image);
void gfx_image(const Image *image, int x, int y, int largeur, int hauteur);
void gfx_image_transparente(const Image *image, int x, int y, int largeur, int hauteur, int opacite);
/* Image recadrée pour remplir exactement le rectangle (sans déformation) */
void gfx_image_remplir(const Image *image, int x, int y, int largeur, int hauteur);

/* Temps, en millisecondes */
unsigned int gfx_temps(void);
void gfx_attendre(unsigned int ms);

/* Entrées. Fermer la fenêtre quitte proprement le programme */
int gfx_evenement(Evenement *ev);                 /* Sans attente : 1 si un événement a été lu */
int gfx_attendre_evenement(Evenement *ev, int ms); /* Attend au plus ms millisecondes */
void gfx_vider_evenements(void);
void gfx_souris(int *x, int *y);
int gfx_touche_enfoncee(SDL_Keycode touche);

/* Saisie de texte (nom du joueur) */
void gfx_saisie_texte(int active);

#endif
