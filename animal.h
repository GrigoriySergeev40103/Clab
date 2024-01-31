#include <stdint.h>

#define MAX_AGE 5
#define DEFAULT_TTL 5

enum AnimalType
{
    A,
    B,
    C
};

struct Position
{
    int8_t x;
    int8_t y;
};

struct Animal
{
    enum AnimalType type;
    int8_t satiety; // time to live
    int8_t age;
};

void RandomizeAnimal(struct Animal* toRandomize);

void MoveUp(struct Position* myself);
void MoveDown(struct Position* myself);
void MoveLeft(struct Position* myself);
void MoveRight(struct Position* myself);

char TypeToChar(enum AnimalType type);