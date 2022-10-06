/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Eric Munson
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019, 2020 Karen Reid
 */

/**
 * CSC369 Tut3 - Output Functions
 *
 * NOTE: This file will be replaced when we run your code.
 *       DO NOT make any changes here.
 */

#include "output.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define UPPER 10000
#define LOWER 500

void output_init()
{
	srand(time(NULL));
}

static void delay()
{
	useconds_t sleep_usec = (rand() % (UPPER - LOWER + 1)) + LOWER;
	usleep(sleep_usec);
}

void print_a1()
{
	delay();
	printf("A1\n");
}

void print_a2()
{
	delay();
	printf("A2\n");
}

void print_b1()
{
	delay();
	printf("B1\n");
}

void print_b2()
{
	delay();
	printf("B2\n");
}

