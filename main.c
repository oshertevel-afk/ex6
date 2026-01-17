/* NAME: OSHER TEVEL
ID: 212695522
EX6
*/

#include <stdio.h>
#include <stdlib.h>
#include "game.h"
#include "utils.h"

typedef void (*ActionFunc)(GameState*);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <player_hp> <base_attack>\n", argv[0]);
        return 1;
    }

    GameState game = {0};
    game.configMaxHp = atoi(argv[1]);
    game.configBaseAttack = atoi(argv[2]);

    ActionFunc actions[] = {NULL, addRoom, initPlayer, playGame};

    int running = 1;
    while (running) {
        printf("\n=== MENU ===\n1.Add Room\n2.Init Player\n3.Play\n4.Exit\n");
        int c = getInt("Choice: ");
        if (c == 4) running = 0;
        else if (c >= 1 && c <= 3) actions[c](&game);
    }

    freeGame(&game);
    return 0;
}
