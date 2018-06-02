//
// Created by modp on 6/2/18.
//

#ifndef RNGS_C_H
#define RNGS_C_H

double Random(void);
void   PlantSeeds(long x);
void   GetSeed(long *x);
void   PutSeed(long x);
void   SelectStream(int index);
int   TestRandom(void);

#endif RNGS_C_H
