/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Eric Munson, Angela Demke Brown
 * Copyright (c) 2021 Eric Munson, Angela Demke Brown
 */
#include <stdio.h>
#include <pthread.h>
#include "sync.h"
#include "output.h"

/*
 * This program is a version of the rendezvous problem.
 * The threads must wait for each other at the rendezvous point (after 
 * calling their first print function). Only when both threads have completed
 * their first print function can either thread proceed to their second
 * print function. 
 * 
 *  You will need to add the code to the functions thread_a
 * and thread_b to ensure that the print of 'A1' always happens before 'B2' and
 * that 'B1' always happens before 'A2'. There should be no enforced order on
 * the 'A1' and 'B1' prints, or between the 'A2' and 'B2'prints. Any order
 * in which 'A1' and 'B1' appear before 'A2' and 'B2' are valid. Specifically,
 * the following orders should all be possible:
 *
 * Legal:      Legal:      Legal:      Legal:
 * A1          A1          B1          B1
 * B1          B1          A1          A1
 * B2          A2          B2          A2
 * A2          B2          A2          B2
 *
 *
 * You may add global variables to this file, but you cannot alter any other
 * file.  You should not need to alter any functions beyond main() (for global
 * initialization), thread_a(), and thread_b() to solve the problem.
 */

pthread_cond_t not_a1;
pthread_cond_t not_b1;
pthread_mutex_t print_lock;
int not_a = 0;
int not_b = 0;


void *thread_a(void *arg)
{
	(void)arg;

	// TODO: Add synchronization and signaling to enforce ordering
	print_a1();
	pthread_mutex_lock(&print_lock);
	not_a = 1;
	/* Rendezvous with thread_b */
	while (not_b == 0) {
		pthread_cond_wait(&not_b1, &print_lock);
	}
	pthread_cond_broadcast(&not_a1);
	pthread_mutex_unlock(&print_lock);
	print_a2();

	return NULL;
}

void *thread_b(void *arg)
{
	(void)arg;

	// TODO: Add synchronization and signaling to enforce ordering.
	print_b1();
	pthread_mutex_lock(&print_lock);
	not_b = 1;
	/* Rendezvous with thread_a */
	while (not_a == 0) {
		pthread_cond_wait(&not_a1, &print_lock);
	}
	pthread_cond_broadcast(&not_b1);
	pthread_mutex_unlock(&print_lock);

	print_b2();

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t a;
	pthread_t b;

	output_init();
	
	// TODO: Add initialization for synchronization variables.

	if (pthread_create(&a, NULL, thread_a, NULL)) {
		perror("pthread_create");
	}

	if (pthread_create(&b, NULL, thread_b, NULL)) {
		perror("pthread_create");
	}

	if (pthread_join(a, NULL)) {
		perror("pthread_join");
	}

	if (pthread_join(b, NULL)) {
		perror("pthread_join");
	}

	return 0;
}

