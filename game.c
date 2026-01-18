#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "utils.h"

/* Constants */
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

#define YES 1

#define ORDER_PRE 1
#define ORDER_IN 2
#define ORDER_POST 3

#define PLAY_MOVE 1
#define PLAY_FIGHT 2
#define PLAY_PICKUP 3
#define PLAY_BAG 4
#define PLAY_DEFEATED 5
#define PLAY_QUIT 6

/* Prototypes */
static void displayMap(GameState* g);

static Room* findRoomById(GameState* g, int id);
static Room* findRoomByCoord(GameState* g, int x, int y);
static void printRoomLegend(GameState* g, int descending);
static void getTargetCoord(const Room* base, int dir, int* outX, int* outY);

static Monster* readMonster(void);
static Item* readItem(void);

static void printCurrentRoom(const GameState* g);

static void doMove(GameState* g);
static void doFight(GameState* g);
static void doPickup(GameState* g);
static void doBag(GameState* g);
static void doDefeated(GameState* g);

static int allRoomsVisited(const GameState* g);
static int anyMonstersLeft(const GameState* g);
static void checkVictory(GameState* g);

static const char* itemTypeToString(ItemType t);
static const char* monsterTypeToString(MonsterType t);

/* Given map display */
static void displayMap(GameState* g) {
	if (!g->rooms) return;

	int minX = 0, maxX = 0, minY = 0, maxY = 0;
	for (Room* r = g->rooms; r; r = r->next) {
		if (r->x < minX) minX = r->x;
		if (r->x > maxX) maxX = r->x;
		if (r->y < minY) minY = r->y;
		if (r->y > maxY) maxY = r->y;
	}

	int width = maxX - minX + 1;
	int height = maxY - minY + 1;

	int** grid = malloc((size_t)height * sizeof(int*));
	for (int i = 0; i < height; i++) {
		grid[i] = malloc((size_t)width * sizeof(int));
		for (int j = 0; j < width; j++) {
			grid[i][j] = -1;
		}
	}

	for (Room* r = g->rooms; r; r = r->next) {
		grid[r->y - minY][r->x - minX] = r->id;
	}

	printf("=== SPATIAL MAP ===\n");
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (grid[i][j] != -1) {
				printf("[%2d]", grid[i][j]);
			} else {
				printf("    ");
			}
		}
		printf("\n");
	}

	for (int i = 0; i < height; i++) {
		free(grid[i]);
	}
	free(grid);
}

/* Item / Monster functions */
void freeMonster(void* data) {
	Monster* m = (Monster*)data;
	free(m->name);
	free(m);
}

void freeItem(void* data) {
	Item* it = (Item*)data;
	free(it->name);
	free(it);
}

int compareItems(void* a, void* b) {
	Item* i1 = (Item*)a;
	Item* i2 = (Item*)b;
	int c = strcmp(i1->name, i2->name);
	if (c != 0) return c;
	if (i1->value != i2->value) return i1->value - i2->value;
	return (int)i1->type - (int)i2->type;
}

int compareMonsters(void* a, void* b) {
	Monster* m1 = (Monster*)a;
	Monster* m2 = (Monster*)b;
	int c = strcmp(m1->name, m2->name);
	if (c != 0) return c;
	if (m1->attack != m2->attack) return m1->attack - m2->attack;
	if (m1->hp != m2->hp) return m1->hp - m2->hp;
	return (int)m1->type - (int)m2->type;
}

void printItem(void* data) {
	Item* it = (Item*)data;
	printf("[%s] %s - Value: %d\n",
	       itemTypeToString(it->type), it->name, it->value);
}

void printMonster(void* data) {
	Monster* m = (Monster*)data;
	printf("[%s] Type: %s, Attack: %d, HP: %d\n",
	       m->name, monsterTypeToString(m->type), m->attack, m->hp);
}

/* Helpers */
static Room* findRoomById(GameState* g, int id) {
	for (Room* r = g->rooms; r; r = r->next) {
		if (r->id == id) return r;
	}
	return NULL;
}

static Room* findRoomByCoord(GameState* g, int x, int y) {
	for (Room* r = g->rooms; r; r = r->next) {
		if (r->x == x && r->y == y) return r;
	}
	return NULL;
}

static void printRoomLegend(GameState* g, int descending) {
	printf("=== ROOM LEGEND ===\n");
	if (descending) {
		for (int id = g->roomCount - 1; id >= 0; id--) {
			Room* r = findRoomById(g, id);
			if (r) {
				printf("ID %d: [M:%c] [I:%c]\n",
				       r->id,
				       r->monster ? 'V' : 'X',
				       r->item ? 'V' : 'X');
			}
		}
	} else {
		for (int id = 0; id < g->roomCount; id++) {
			Room* r = findRoomById(g, id);
			if (r) {
				printf("ID %d: [M:%c] [I:%c]\n",
				       r->id,
				       r->monster ? 'V' : 'X',
				       r->item ? 'V' : 'X');
			}
		}
	}
	printf("===================\n");
}

static void getTargetCoord(const Room* base, int dir, int* outX, int* outY) {
	*outX = base->x;
	*outY = base->y;

	if (dir == DIR_UP) (*outY)--;
	else if (dir == DIR_DOWN) (*outY)++;
	else if (dir == DIR_LEFT) (*outX)--;
	else if (dir == DIR_RIGHT) (*outX)++;
}

static Monster* readMonster(void) {
	Monster* m = malloc(sizeof(Monster));
	m->name = getString("Monster name: ");
	m->type = (MonsterType)getInt("Type (0-4): ");
	m->hp = getInt("HP: ");
	m->maxHp = m->hp;
	m->attack = getInt("Attack: ");
	return m;
}

static Item* readItem(void) {
	Item* it = malloc(sizeof(Item));
	it->name = getString("Item name: ");
	it->type = (ItemType)getInt("Type (0=Armor, 1=Sword): ");
	it->value = getInt("Value: ");
	return it;
}

static void printCurrentRoom(const GameState* g) {
	Room* r = g->player->currentRoom;
	printf("--- Room %d ---\n", r->id);
	if (r->monster) {
		printf("Monster: %s (HP:%d)\n", r->monster->name, r->monster->hp);
	}
	if (r->item) {
		printf("Item: %s\n", r->item->name);
	}
	printf("HP: %d/%d\n", g->player->hp, g->player->maxHp);
}

/* Game actions */
static void doMove(GameState* g) {
	Room* cur = g->player->currentRoom;
	if (cur->monster) {
		printf("Kill monster first\n");
		return;
	}
	int dir = getInt("Direction (0=Up,1=Down,2=Left,3=Right): ");
	int nx, ny;
	getTargetCoord(cur, dir, &nx, &ny);
	if (!findRoomByCoord(g, nx, ny)) {
		printf("No room there\n");
		return;
	}
	g->player->currentRoom = findRoomByCoord(g, nx, ny);
	g->player->currentRoom->visited = 1;
}

static void doFight(GameState* g) {
	Player* p = g->player;
	Monster* m = p->currentRoom->monster;

	if (!m) {
		printf("No monster\n");
		return;
	}

	while (p->hp > 0 && m->hp > 0) {
		m->hp -= p->baseAttack;
		if (m->hp < 0) m->hp = 0;
		printf("You deal %d damage. Monster HP: %d\n", p->baseAttack, m->hp);
		if (m->hp == 0) break;
		p->hp -= m->attack;
		if (p->hp < 0) p->hp = 0;
		printf("Monster deals %d damage. Your HP: %d\n", m->attack, p->hp);
	}

	if (p->hp == 0) {
		freeGame(g);
		printf("--- YOU DIED ---\n");
		exit(0);
	}

	printf("Monster defeated!\n");
	p->defeatedMonsters->root =
		bstInsert(p->defeatedMonsters->root, m, compareMonsters);
	p->currentRoom->monster = NULL;
}

static void doPickup(GameState* g) {
	Room* r = g->player->currentRoom;
	if (r->monster) {
		printf("Kill monster first\n");
		return;
	}
	if (!r->item) {
		printf("No item here\n");
		return;
	}
	if (bstFind(g->player->bag->root, r->item, compareItems)) {
		printf("Duplicate item.\n");
		return;
	}
	g->player->bag->root =
		bstInsert(g->player->bag->root, r->item, compareItems);
	printf("Picked up %s\n", r->item->name);
	r->item = NULL;
}

static void doBag(GameState* g) {
	printf("=== INVENTORY ===\n");
	printf("1.Preorder 2.Inorder 3.Postorder\n");
	int c = getInt("");
	if (!g->player->bag->root) return;
	if (c == ORDER_PRE) bstPreorder(g->player->bag->root, printItem);
	else if (c == ORDER_IN) bstInorder(g->player->bag->root, printItem);
	else if (c == ORDER_POST) bstPostorder(g->player->bag->root, printItem);
}

static void doDefeated(GameState* g) {
	printf("=== DEFEATED MONSTERS ===\n");
	printf("1.Preorder 2.Inorder 3.Postorder\n");
	int c = getInt("");
	if (!g->player->defeatedMonsters->root) return;
	if (c == ORDER_PRE) bstPreorder(g->player->defeatedMonsters->root, printMonster);
	else if (c == ORDER_IN) bstInorder(g->player->defeatedMonsters->root, printMonster);
	else if (c == ORDER_POST) bstPostorder(g->player->defeatedMonsters->root, printMonster);
}

/* Victory */
static int allRoomsVisited(const GameState* g) {
	for (Room* r = g->rooms; r; r = r->next) {
		if (!r->visited) return 0;
	}
	return 1;
}

static int anyMonstersLeft(const GameState* g) {
	for (Room* r = g->rooms; r; r = r->next) {
		if (r->monster) return 1;
	}
	return 0;
}

static void checkVictory(GameState* g) {
	if (allRoomsVisited(g) && !anyMonstersLeft(g)) {
		printf("***************************************\n");
		printf("             VICTORY!                  \n");
		printf(" All rooms explored. All monsters defeated. \n");
		printf("***************************************\n");
		freeGame(g);
		exit(0);
	}
}

/* Public API */
void addRoom(GameState* g) {
	if (!g->rooms) {
		Room* r = malloc(sizeof(Room));
		r->id = 0;
		r->x = 0;
		r->y = 0;
		r->visited = 0;
		r->monster = NULL;
		r->item = NULL;
		r->next = NULL;
		g->rooms = r;
		g->roomCount = 1;
		printf("Created room 0 at (0,0)\n");
		return;
	}

	displayMap(g);
	printRoomLegend(g, 0);

	int attachId = getInt("Attach to room ID: ");
	int dir = getInt("Direction (0=Up,1=Down,2=Left,3=Right): ");

	Room* base = findRoomById(g, attachId);
	int nx, ny;
	getTargetCoord(base, dir, &nx, &ny);

	if (findRoomByCoord(g, nx, ny)) {
		printf("Room exists there\n");
		return;
	}

	Room* r = malloc(sizeof(Room));
	r->id = g->roomCount;
	r->x = nx;
	r->y = ny;
	r->visited = 0;
	r->monster = NULL;
	r->item = NULL;
	r->next = g->rooms;
	g->rooms = r;
	g->roomCount++;

	if (getInt("Add monster? (1=Yes, 0=No): ") == YES) {
		r->monster = readMonster();
	}
	if (getInt("Add item? (1=Yes, 0=No): ") == YES) {
		r->item = readItem();
	}

	printf("Created room %d at (%d,%d)\n", r->id, r->x, r->y);
}

void initPlayer(GameState* g) {
	if (!g->rooms) {
		printf("Create rooms first\n");
		return;
	}

	Player* p = malloc(sizeof(Player));
	p->maxHp = g->configMaxHp;
	p->hp = p->maxHp;
	p->baseAttack = g->configBaseAttack;
	p->bag = createBST(compareItems, printItem, freeItem);
	p->defeatedMonsters = createBST(compareMonsters, printMonster, freeMonster);
	p->currentRoom = findRoomById(g, 0);
	p->currentRoom->visited = 1;
	g->player = p;
}

void playGame(GameState* g) {
	if (!g->player) {
		printf("Init player first\n");
		return;
	}

	while (1) {
		displayMap(g);
		printRoomLegend(g, 1);
		printCurrentRoom(g);
		checkVictory(g);

		printf("1.Move 2.Fight 3.Pickup 4.Bag 5.Defeated 6.Quit\n");
		int c = getInt("");

		if (c == PLAY_QUIT) return;
		else if (c == PLAY_MOVE) doMove(g);
		else if (c == PLAY_FIGHT) doFight(g);
		else if (c == PLAY_PICKUP) doPickup(g);
		else if (c == PLAY_BAG) doBag(g);
		else if (c == PLAY_DEFEATED) doDefeated(g);
	}
}

void freeGame(GameState* g) {
	if (!g) return;

	if (g->player) {
		bstFree(g->player->bag->root, g->player->bag->freeData);
		free(g->player->bag);
		bstFree(g->player->defeatedMonsters->root,
		        g->player->defeatedMonsters->freeData);
		free(g->player->defeatedMonsters);
		free(g->player);
	}

	Room* r = g->rooms;
	while (r) {
		Room* next = r->next;
		if (r->monster) freeMonster(r->monster);
		if (r->item) freeItem(r->item);
		free(r);
		r = next;
	}

	g->rooms = NULL;
	g->player = NULL;
	g->roomCount = 0;
}

/* String helpers */
static const c
