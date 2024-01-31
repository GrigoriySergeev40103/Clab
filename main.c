#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "animal.h"

#define FIELD_HEIGHT 15
#define FIELD_WIDTH 15

#define ORPHANAGE_CAPACITY 10

struct FieldCell
{
    pthread_t thread;
    struct Animal animal;
    pthread_mutex_t cellMutex;
};

void* AnimalActivity(void* arg);

pthread_attr_t animalThreadAttr;

pthread_mutex_t field_mutex = PTHREAD_MUTEX_INITIALIZER;
struct FieldCell animal_field[FIELD_WIDTH][FIELD_HEIGHT];

pthread_mutex_t orphanageMutex = PTHREAD_MUTEX_INITIALIZER;
struct Animal orphanage[ORPHANAGE_CAPACITY];
size_t orphansCount;

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);

    pthread_attr_init(&animalThreadAttr);
    pthread_attr_setdetachstate(&animalThreadAttr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_lock(&field_mutex);

    for (size_t i = 0; i < FIELD_WIDTH; i++)
    {
        for (size_t j = 0; j < FIELD_HEIGHT; j++)
        {
            pthread_mutex_init(&animal_field[i][j].cellMutex, NULL);

            int shouldGenerate = rand() % 2;

            if(shouldGenerate)
            {
                RandomizeAnimal(&animal_field[i][j].animal);
                struct Position* pos = malloc(sizeof(struct Position));
                pos->x = i;
                pos->y = j;
                pthread_create(&animal_field[i][j].thread, &animalThreadAttr, &AnimalActivity, pos);
            }
        }
    }

    printf("Animal game began...\n");

    pthread_mutex_unlock(&field_mutex);

    pthread_exit(NULL);

    return 0;
}

void TryDeployFromOrphanage(int8_t x, int8_t y)
{
    pthread_mutex_lock(&orphanageMutex);

    if(orphansCount != 0)
    {
        struct Position* pos = malloc(sizeof(struct Position));
        pos->x = x;
        pos->y = y;
        animal_field[x][y].animal = orphanage[orphansCount];
        printf("Deploying child to [%d, %d]\n", x, y);
        pthread_create(&animal_field[x][y].thread, &animalThreadAttr, &AnimalActivity, pos);
        orphansCount--;
    }

    pthread_mutex_unlock(&orphanageMutex);
}

void OnAnimalDiedBeforeTurn(void* arg)
{
    pthread_mutex_unlock(&field_mutex);
    struct Position* cleanupPos = arg;
    pthread_mutex_unlock(&animal_field[cleanupPos->x][cleanupPos->y].cellMutex);
}

struct CleanUpArg
{
    pthread_mutex_t* cell1Mutex;
    pthread_mutex_t* cell2Mutex;
};

void OnAnimalTurnEndClenup(void* arg)
{
    struct CleanUpArg* cleanupArg = arg;
    pthread_mutex_unlock(cleanupArg->cell1Mutex);
    pthread_mutex_unlock(cleanupArg->cell2Mutex);
}

typedef void (*MoveFunc)(struct Position*);

void* AnimalActivity(void* arg)
{
    MoveFunc possibleMoves[4];
    int movesCount;
    struct Position myPos = *(struct Position*)arg;
    free(arg); // no longer needed
    struct Position otherPos = myPos;

    struct Animal* me;
    struct FieldCell* otherCell;
    struct FieldCell* myCell;
    struct Animal* otherAnimal;

    while (1)
    {
        pthread_cleanup_push(&OnAnimalDiedBeforeTurn, &myPos);

        pthread_mutex_lock(&field_mutex);

        myCell = &animal_field[myPos.x][myPos.y];

        pthread_mutex_lock(&myCell->cellMutex);

        pthread_testcancel(); // if we've been eaten

        movesCount = 0;

        me = &myCell->animal;

        if(me->satiety == 0 || me->age >= MAX_AGE)
        {
            // deadge and sadge
            myCell->thread = 0;
            printf("(T: %c, H: %d, A: %d) x @ [%d, %d]\n", 
                    TypeToChar(me->type), me->satiety, me->age, myPos.x, myPos.y);
            
            //-----------------Spawn a child from orphanage in our stead-----------------
            TryDeployFromOrphanage(myPos.x, myPos.y);
            //-----------------Spawn a child from orphanage in our stead-----------------

            pthread_exit(0); // possible seg fault later(due to returning 0)
        }

        pthread_cleanup_pop(0);

        if(myPos.x + 1 < FIELD_WIDTH)
        {
            possibleMoves[movesCount] = MoveRight;
            movesCount++;
        }
        if(myPos.y + 1 < FIELD_HEIGHT)
        {
            possibleMoves[movesCount] = MoveUp;
            movesCount++;
        }
        if(myPos.x > 0)
        {
            possibleMoves[movesCount] = MoveLeft;
            movesCount++;
        }
        if(myPos.y > 0)
        {
            possibleMoves[movesCount] = MoveDown;
            movesCount++;
        }

        // 0..movesCount - 1
        int directionChoice = rand() % movesCount;

        (*(possibleMoves[directionChoice]))(&otherPos); // at this  point otherPos == myPos

        otherCell = &animal_field[otherPos.x][otherPos.y];

        pthread_mutex_lock(&otherCell->cellMutex);
        pthread_mutex_unlock(&field_mutex);

        struct CleanUpArg cleanupArg;
        cleanupArg.cell1Mutex = &animal_field[myPos.x][myPos.y].cellMutex;
        cleanupArg.cell2Mutex = &animal_field[otherPos.x][otherPos.y].cellMutex;
        pthread_cleanup_push(&OnAnimalTurnEndClenup, &cleanupArg);

        otherAnimal = &otherCell->animal;

        if(otherCell->thread != 0)
        {
            if(otherAnimal->type == me->type)
            {
                // create a new animal and remain in our cells
                pthread_mutex_lock(&orphanageMutex);

                if(orphansCount < ORPHANAGE_CAPACITY)
                {
                    // Create new animal
                    RandomizeAnimal(&orphanage[orphansCount]);
                    orphansCount++;
                    printf("(T: %c, H: %d, A: %d) @ [%d, %d] + [%d, %d]\n", 
                        TypeToChar(me->type), me->satiety, me->age, myPos.x, myPos.y, otherPos.x, otherPos.y);
                }
                else
                {
                    printf("(T: %c, H: %d, A: %d) @ [%d, %d] x+ [%d, %d]\n", 
                        TypeToChar(me->type), me->satiety, me->age, myPos.x, myPos.y, otherPos.x, otherPos.y);
                }

                pthread_mutex_unlock(&orphanageMutex);

                otherPos = myPos; // since we won't move there(parents just remian in their places)
            }
            else
            {
                int type = me->type + 1;
                // if we loop around(pretty much just doing modular addition based on animal type enum)
                if(type == 3)
                {
                    type = 0;
                }

                // then we eat them
                if(type == otherAnimal->type)
                {
                    printf("(T: %c, H: %d, A: %d) @ [%d, %d] < [%d, %d]\n", 
                        TypeToChar(me->type), me->satiety, me->age, myPos.x, myPos.y, otherPos.x, otherPos.y);

                    pthread_cancel(otherCell->thread);

                    me->satiety = DEFAULT_TTL;
                    *otherAnimal = *me;
                    me = otherAnimal;
                    otherCell->thread = myCell->thread;
                    myCell->thread = 0;

                    TryDeployFromOrphanage(myPos.x, myPos.y);
                }
                else // they eat us
                {
                    printf("(T: %c, H: %d, A: %d) @ [%d, %d] > [%d, %d]\n", 
                        TypeToChar(me->type), me->satiety, me->age, myPos.x, myPos.y, otherPos.x, otherPos.y);

                    otherAnimal->satiety = DEFAULT_TTL;
                    animal_field[myPos.x][myPos.y].thread = 0;

                    pthread_exit(0);
                }
            }
        }
        else
        {
            printf("(T: %c, H: %d, A: %d) @ [%d, %d] -> [%d, %d]\n", 
                        TypeToChar(me->type), me->satiety, me->age, myPos.x, myPos.y, otherPos.x, otherPos.y);

            *otherAnimal = *me;
            me = otherAnimal;

            otherCell->thread = myCell->thread;
            myCell->thread = 0;
        }

        me->age++;
        me->satiety--;
        pthread_cleanup_pop(1);

        myPos = otherPos;
    }
}