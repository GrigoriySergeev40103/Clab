#include "animal.h"
#include <stdlib.h>

void RandomizeAnimal(struct Animal* toRandomize)
{
    toRandomize->age = 0;
    toRandomize->satiety = DEFAULT_TTL;

    // 0..2
    int type = rand() % 3;
    toRandomize->type = type;
}

void MoveUp(struct Position* myself)
{
    myself->y++;
}

void MoveRight(struct Position* myself)
{
    myself->x++;
}

void MoveDown(struct Position* myself)
{
    myself->y--;
}

void MoveLeft(struct Position* myself)
{
    myself->x--;
}

char TypeToChar(enum AnimalType type)
{
    return 65 + type;
}

void ConfigureNewAnimal(struct Animal* toConfigure, enum AnimalType type)
{
    toConfigure->age = 0;
    toConfigure->satiety = DEFAULT_TTL;
    toConfigure->type = type;
}