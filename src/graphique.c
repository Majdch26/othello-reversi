#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_image.h>
#include "graphique.h"

static SDL_Window *fenetre = NULL;
static SDL_Renderer *rendu = NULL;

/* ------------------------------------------------------------------------- */
/* Cache des textes : créer une texture de texte coûte cher, et ce sont       */
/* presque toujours les mêmes textes qui sont affichés d'une image à l'autre. */
/* ------------------------------------------------------------------------- */

#define TAILLE_CACHE 96
#define LONGUEUR_TEXTE 96

typedef struct {
    char texte[LONGUEUR_TEXTE];
    Police *police;
    Couleur couleur;
    SDL_Texture *texture;
    int largeur, hauteur;
    unsigned int utilise;  /* Date de dernière utilisation */
} EntreeCache;

static EntreeCache cache[TAILLE_CACHE];
static unsigned int horloge_cache = 0;

static void vider_cache(void) {
    int i;
    for (i = 0; i < TAILLE_CACHE; i++) {
        if (cache[i].texture != NULL) {
            SDL_DestroyTexture(cache[i].texture);
        }
        cache[i].texture = NULL;
        cache[i].texte[0] = '\0';
    }
}

/* ------------------------------------------------------------------------- */

Couleur rgba(int r, int v, int b, int a) {
    return ((Couleur)(r & 255) << 24) | ((Couleur)(v & 255) << 16)
         | ((Couleur)(b & 255) << 8) | (Couleur)(a & 255);
}

void composantes(Couleur c, int *r, int *v, int *b, int *a) {
    *r = (int)((c >> 24) & 255);
    *v = (int)((c >> 16) & 255);
    *b = (int)((c >> 8) & 255);
    *a = (int)(c & 255);
}

static SDL_Color couleur_sdl(Couleur c) {
    SDL_Color s;
    int r, v, b, a;
    composantes(c, &r, &v, &b, &a);
    s.r = (Uint8)r;
    s.g = (Uint8)v;
    s.b = (Uint8)b;
    s.a = (Uint8)a;
    return s;
}

int gfx_ouvrir(const char *titre, int largeur, int hauteur) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL : %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Erreur SDL_ttf : %s\n", TTF_GetError());
        return -1;
    }
    if ((IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG)) == 0) {
        fprintf(stderr, "Erreur SDL_image : %s\n", IMG_GetError());
        return -1;
    }
    fenetre = SDL_CreateWindow(titre, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               largeur, hauteur, SDL_WINDOW_SHOWN);
    if (fenetre == NULL) {
        fprintf(stderr, "Impossible de créer la fenêtre : %s\n", SDL_GetError());
        return -1;
    }
    rendu = SDL_CreateRenderer(fenetre, -1, SDL_RENDERER_ACCELERATED);
    if (rendu == NULL) {
        rendu = SDL_CreateRenderer(fenetre, -1, 0); /* Rendu logiciel en secours */
    }
    if (rendu == NULL) {
        fprintf(stderr, "Impossible de créer le rendu : %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetRenderDrawBlendMode(rendu, SDL_BLENDMODE_BLEND);
    memset(cache, 0, sizeof(cache));
    return 0;
}

void gfx_fermer(void) {
    vider_cache();
    if (rendu != NULL) {
        SDL_DestroyRenderer(rendu);
    }
    if (fenetre != NULL) {
        SDL_DestroyWindow(fenetre);
    }
    rendu = NULL;
    fenetre = NULL;
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void gfx_actualiser(void) {
    SDL_RenderPresent(rendu);
}

static void choisir_couleur(Couleur c) {
    int r, v, b, a;
    composantes(c, &r, &v, &b, &a);
    SDL_SetRenderDrawColor(rendu, (Uint8)r, (Uint8)v, (Uint8)b, (Uint8)a);
}

void gfx_rect_plein(int x, int y, int largeur, int hauteur, Couleur c) {
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = largeur;
    r.h = hauteur;
    choisir_couleur(c);
    SDL_RenderFillRect(rendu, &r);
}

void gfx_disque(int cx, int cy, int rayon, Couleur c) {
    int dy, dx;
    choisir_couleur(c);
    for (dy = -rayon; dy <= rayon; dy++) {
        dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= rayon * rayon) {
            dx++;
        }
        SDL_RenderDrawLine(rendu, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void gfx_cercle(int cx, int cy, int rayon, int epaisseur, Couleur c) {
    int dy, dx_ext, dx_int, r_int = rayon - epaisseur;
    choisir_couleur(c);
    for (dy = -rayon; dy <= rayon; dy++) {
        dx_ext = 0;
        while ((dx_ext + 1) * (dx_ext + 1) + dy * dy <= rayon * rayon) {
            dx_ext++;
        }
        if (dy * dy >= r_int * r_int) {
            SDL_RenderDrawLine(rendu, cx - dx_ext, cy + dy, cx + dx_ext, cy + dy);
        } else {
            dx_int = 0;
            while ((dx_int + 1) * (dx_int + 1) + dy * dy < r_int * r_int) {
                dx_int++;
            }
            SDL_RenderDrawLine(rendu, cx - dx_ext, cy + dy, cx - dx_int, cy + dy);
            SDL_RenderDrawLine(rendu, cx + dx_int, cy + dy, cx + dx_ext, cy + dy);
        }
    }
}

void gfx_rect(int x, int y, int largeur, int hauteur, Couleur c) {
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = largeur;
    r.h = hauteur;
    choisir_couleur(c);
    SDL_RenderDrawRect(rendu, &r);
}

/* ------------------------------------------------------------------------- */
/* Texte                                                                      */
/* ------------------------------------------------------------------------- */

Police *gfx_charger_police(const char *chemin, int taille) {
    return TTF_OpenFont(chemin, taille);
}

void gfx_liberer_police(Police *police) {
    if (police != NULL) {
        TTF_CloseFont(police);
    }
}

/* Renvoie l'entrée du cache correspondant au texte (en la créant au besoin) */
static EntreeCache *texte_en_cache(const char *texte, Police *police, Couleur c) {
    int i, ancienne = 0;
    SDL_Surface *surface;
    EntreeCache *e;

    horloge_cache++;
    for (i = 0; i < TAILLE_CACHE; i++) {
        e = &cache[i];
        if (e->texture != NULL && e->police == police && e->couleur == c
            && strcmp(e->texte, texte) == 0) {
            e->utilise = horloge_cache;
            return e;
        }
        if (cache[i].utilise < cache[ancienne].utilise) {
            ancienne = i;
        }
    }

    /* Pas trouvé : on remplace l'entrée la moins récemment utilisée */
    surface = TTF_RenderUTF8_Blended(police, texte, couleur_sdl(c));
    if (surface == NULL) {
        return NULL;
    }
    e = &cache[ancienne];
    if (e->texture != NULL) {
        SDL_DestroyTexture(e->texture);
    }
    e->texture = SDL_CreateTextureFromSurface(rendu, surface);
    e->largeur = surface->w;
    e->hauteur = surface->h;
    SDL_FreeSurface(surface);
    strncpy(e->texte, texte, LONGUEUR_TEXTE - 1);
    e->texte[LONGUEUR_TEXTE - 1] = '\0';
    e->police = police;
    e->couleur = c;
    e->utilise = horloge_cache;
    return e;
}

void gfx_texte(int x, int y, const char *texte, Police *police, Couleur c) {
    SDL_Rect dest;
    SDL_Surface *surface;
    SDL_Texture *texture;
    EntreeCache *e;

    if (texte == NULL || texte[0] == '\0' || police == NULL) {
        return;
    }
    if (strlen(texte) < LONGUEUR_TEXTE) {
        e = texte_en_cache(texte, police, c);
        if (e != NULL && e->texture != NULL) {
            dest.x = x;
            dest.y = y;
            dest.w = e->largeur;
            dest.h = e->hauteur;
            SDL_RenderCopy(rendu, e->texture, NULL, &dest);
        }
        return;
    }
    /* Texte trop long pour le cache : rendu direct */
    surface = TTF_RenderUTF8_Blended(police, texte, couleur_sdl(c));
    if (surface == NULL) {
        return;
    }
    texture = SDL_CreateTextureFromSurface(rendu, surface);
    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;
    SDL_RenderCopy(rendu, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void gfx_taille_texte(const char *texte, Police *police, int *largeur, int *hauteur) {
    if (police == NULL || TTF_SizeUTF8(police, texte, largeur, hauteur) != 0) {
        *largeur = 0;
        *hauteur = 0;
    }
}

/* ------------------------------------------------------------------------- */
/* Images                                                                     */
/* ------------------------------------------------------------------------- */

Image *gfx_charger_image(const char *chemin) {
    Image *image;
    SDL_Texture *texture = IMG_LoadTexture(rendu, chemin);
    if (texture == NULL) {
        return NULL;
    }
    image = malloc(sizeof(Image));
    if (image == NULL) {
        SDL_DestroyTexture(texture);
        return NULL;
    }
    image->texture = texture;
    SDL_QueryTexture(texture, NULL, NULL, &image->largeur, &image->hauteur);
    return image;
}

void gfx_liberer_image(Image *image) {
    if (image != NULL) {
        SDL_DestroyTexture(image->texture);
        free(image);
    }
}

void gfx_image(const Image *image, int x, int y, int largeur, int hauteur) {
    SDL_Rect dest;
    if (image == NULL) {
        return;
    }
    dest.x = x;
    dest.y = y;
    dest.w = largeur > 0 ? largeur : image->largeur;
    dest.h = hauteur > 0 ? hauteur : image->hauteur;
    SDL_RenderCopy(rendu, image->texture, NULL, &dest);
}

void gfx_image_transparente(const Image *image, int x, int y, int largeur, int hauteur, int opacite) {
    if (image == NULL) {
        return;
    }
    SDL_SetTextureAlphaMod(image->texture, (Uint8)opacite);
    gfx_image(image, x, y, largeur, hauteur);
    SDL_SetTextureAlphaMod(image->texture, 255);
}

void gfx_image_remplir(const Image *image, int x, int y, int largeur, int hauteur) {
    SDL_Rect source, dest;
    if (image == NULL) {
        return;
    }
    /* On garde la plus grande zone de l'image ayant les proportions voulues */
    if ((long)image->largeur * hauteur > (long)image->hauteur * largeur) {
        source.h = image->hauteur;
        source.w = (int)((long)image->hauteur * largeur / hauteur);
    } else {
        source.w = image->largeur;
        source.h = (int)((long)image->largeur * hauteur / largeur);
    }
    source.x = (image->largeur - source.w) / 2;
    source.y = (image->hauteur - source.h) / 2;
    dest.x = x;
    dest.y = y;
    dest.w = largeur;
    dest.h = hauteur;
    SDL_RenderCopy(rendu, image->texture, &source, &dest);
}

/* ------------------------------------------------------------------------- */
/* Temps et événements                                                        */
/* ------------------------------------------------------------------------- */

unsigned int gfx_temps(void) {
    return (unsigned int)SDL_GetTicks();
}

void gfx_attendre(unsigned int ms) {
    SDL_Delay(ms);
}

/* Convertit un événement SDL. Renvoie 1 s'il nous intéresse */
static int convertir(const SDL_Event *e, Evenement *ev) {
    ev->type = EV_AUCUN;
    switch (e->type) {
    case SDL_QUIT:
        gfx_fermer();
        exit(EXIT_SUCCESS);
    case SDL_KEYDOWN:
        if (e->key.repeat) {
            return 0; /* La répétition des touches est gérée par le jeu */
        }
        ev->type = EV_TOUCHE;
        ev->touche = e->key.keysym.sym;
        return 1;
    case SDL_TEXTINPUT:
        ev->type = EV_TEXTE;
        strncpy(ev->texte, e->text.text, sizeof(ev->texte) - 1);
        ev->texte[sizeof(ev->texte) - 1] = '\0';
        return 1;
    case SDL_MOUSEBUTTONDOWN:
        if (e->button.button != SDL_BUTTON_LEFT) {
            return 0;
        }
        ev->type = EV_CLIC;
        ev->x = e->button.x;
        ev->y = e->button.y;
        return 1;
    default:
        return 0;
    }
}

int gfx_evenement(Evenement *ev) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (convertir(&e, ev)) {
            return 1;
        }
    }
    ev->type = EV_AUCUN;
    return 0;
}

int gfx_attendre_evenement(Evenement *ev, int ms) {
    SDL_Event e;
    unsigned int fin = gfx_temps() + (unsigned int)ms;
    int reste = ms;

    while (reste >= 0 && SDL_WaitEventTimeout(&e, reste)) {
        if (convertir(&e, ev)) {
            return 1;
        }
        reste = (int)(fin - gfx_temps());
    }
    ev->type = EV_AUCUN;
    return 0;
}

void gfx_vider_evenements(void) {
    Evenement ev;
    while (gfx_evenement(&ev)) {
    }
}

void gfx_souris(int *x, int *y) {
    SDL_GetMouseState(x, y);
}

int gfx_touche_enfoncee(SDL_Keycode touche) {
    const Uint8 *etat = SDL_GetKeyboardState(NULL);
    return etat[SDL_GetScancodeFromKey(touche)] != 0;
}

void gfx_saisie_texte(int active) {
    if (active) {
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }
}
