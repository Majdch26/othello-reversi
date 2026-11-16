# Reversi / Othello en C (C89) avec SDL2
#
#   make          compile le jeu (bin/othello) et le tournoi entre IA (bin/tournoi)
#   make run      lance le jeu en mode graphique
#   make console  lance le jeu dans le terminal
#   make test     lance les tests unitaires
#   make clean    supprime les fichiers générés

CC      = gcc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic -O2
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_image 2>/dev/null || sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl2 SDL2_ttf SDL2_image 2>/dev/null || echo "$$(sdl2-config --libs 2>/dev/null) -lSDL2_ttf -lSDL2_image")

SRC   = src
BUILD = build
BIN   = bin

# Règles du jeu et IA : aucune dépendance graphique
MOTEUR    = plateau partie ia
GRAPHIQUE = interface graphique main
TERMINAL  = console

OBJ_MOTEUR    = $(addprefix $(BUILD)/, $(addsuffix .o, $(MOTEUR)))
OBJ_GRAPHIQUE = $(addprefix $(BUILD)/, $(addsuffix .o, $(GRAPHIQUE)))
OBJ_TERMINAL  = $(addprefix $(BUILD)/, $(addsuffix .o, $(TERMINAL)))

.PHONY: all run console test clean

all: $(BIN)/othello $(BIN)/tournoi

$(BIN)/othello: $(OBJ_MOTEUR) $(OBJ_TERMINAL) $(OBJ_GRAPHIQUE) | $(BIN)
	$(CC) $^ -o $@ $(SDL_LIBS) -lm

$(BIN)/tournoi: $(BUILD)/tournoi.o $(OBJ_MOTEUR) | $(BIN)
	$(CC) $^ -o $@

$(BIN)/test_othello: $(BUILD)/test_othello.o $(OBJ_MOTEUR) | $(BIN)
	$(CC) $^ -o $@

$(OBJ_GRAPHIQUE): $(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -MMD -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILD)/test_othello.o: tests/test_othello.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(SRC) -MMD -c $< -o $@

run: $(BIN)/othello
	./$(BIN)/othello

console: $(BIN)/othello
	./$(BIN)/othello -mode Console

test: $(BIN)/test_othello
	./$(BIN)/test_othello

$(BUILD) $(BIN):
	mkdir -p $@

clean:
	rm -rf $(BUILD) $(BIN)

-include $(wildcard $(BUILD)/*.d)
