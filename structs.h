#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>


#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_CLIENTS 4
#define MAX_BEASTS 4
#define MAX_ENTITY 100

#define TICKRATE 	1000000 / 5
#define TIMEOUT  	2

#define MAP_WIDTH 51
#define MAP_HEIGHT 25
#define FOV 4


const wchar_t ref_map[MAP_HEIGHT + 1][MAP_WIDTH + 1] = {
	L"███████████████████████████████████████████████████",
	L"█   █       █   #####       █         █       █   █",
	L"█ █ ███ ███ ███████████ ███ █ ███████ ███ █████   █",
	L"█ █   █ █ █           █ █ █   █     █     █   █   █",
	L"█ ███ █ █ ███###█████ █ █ █████ █████████████ ███ █",
	L"█ █ █   █           █ █ █ ##  █       █       █ █ █",
	L"█ █ █████ ███ ███████ █ █ █ ███ ███ ███ ███ █ █ █ █",
	L"█ █         █ █       █ █ █     █   █   █ █ █   █ █",
	L"█ █ ███████ ███ ███████ █████ ███ ███ ███ █ ███ █ █",
	L"█ █ █     █   █   █     █   █   █         █ █ █ █ █",
	L"█ ███ ███ ███ ███ ███ ███ █ ███ ███████████ █ █ █ █",
	L"█ █   █       █ █   █     █ █   █ █       █ █   █ █",
	L"█ █ ██████#██ █ ███ ███ ███ ███ █ █ █████ █ █ ███ █",
	L"█ █    #█   █ █   █   █   █   █   █ █     █ █ █   █",
	L"█ █ █ ##█ ███ ███ ███ ███████ ███ █ ███ ███ █ █ ███",
	L"█ █ █## █    #  █   █ █  ###  █   █   █     █ █ █ █",
	L"█ █ █#  ███████ █ █ █ █ ██#████ █████ ███████ █ █ █",
	L"█ █ █#      █   █ █ █   █     █   █ █       ##█   █",
	L"█ █████████ █ ███ ███████ ███████ █ █████ █ ##███ █",
	L"█ █#      █ █     █     █       █   █   █ █ ##  █ █",
	L"█ █ █████ █ ███████ █ ███ █████ ███ █ █ ███#█████ █",
	L"█###█     █         █     █## █     █ █   █###### █",
	L"█ ███ █████████████████████#█ ███████ ███ █#    # █",
	L"█   █                 ######█##         █    ##   █",
	L"███████████████████████████████████████████████████"
};


struct point_t {
	int x;
	int y;
};

int point_cmp(struct point_t a, struct point_t b) {
	if (a.x == b.x && a.y == b.y) {
		return 0;
	}
	return 1;
}

struct client_data_t {
	int 				 	id;
	struct point_t 		 	pos;
	int  				 	act;
	int 					pid;
	int 					type;

	// DATA
	int 					deaths;
	int 					payload;
	int 					score;
	int 					bush;
	struct point_t			camp;
	int 					spid;
	int 					round;

	// MAP
	wchar_t map[2*FOV + 1][2*FOV + 2];

	// SERVER RESERVED
	pthread_mutex_t 	 	lock;
	sem_t					*tick;
};


struct beast_data_t {
	// DATA
	int 				 	id;
	int 					bush;
	struct point_t 		 	pos;
	int  				 	act;
	int 					see;

	// MAP
	wchar_t map[2*FOV + 1][2*FOV + 2];

	// SERVER RESERVED
	pthread_mutex_t 	 	lock;
};

struct entity_t {
	struct point_t pos;
	int value;
	wchar_t type;
};

struct server_data_t {
	// CLIENTS
	pthread_t 				client_thread[MAX_CLIENTS];
	struct client_data_t 	client[MAX_CLIENTS];

	// BEASTS
	pthread_t 				beast_thread[MAX_CLIENTS];
	struct beast_data_t 	beast[MAX_BEASTS];
	
	// RNG
	struct point_t 			rng[MAP_WIDTH*MAP_HEIGHT];
	int 		   			rng_len;

	// ENTITY TABLE
	struct entity_t			ent[MAX_ENTITY];

	// VARIABLES
	int 					round;
	int 					pid;
	struct point_t 			camp;
	int 					act;


	// GLOBAL TICK
	sem_t					tick;
	// BEAST TICK
	sem_t					beast_tick;
};


int server_data_init(struct server_data_t *data) {
	if (data == NULL) {
		return 1;
	}
	if (sem_init(&data->tick, 0, 0)) {
		return -1;
	}
	if (sem_init(&data->beast_tick, 0, 0)) {
		return -1;
	}
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		// CLIENT OFF
		data->client[i].id = -1;
		data->client[i].pid = -1;
		// DEFAULT POS
		data->client[i].pos.x = -1;
		data->client[i].pos.y = -1;
		// DEFAULT ACT
		data->client[i].act = 0;
		// DEFAULT STATS
		data->client[i].deaths = 0;
		data->client[i].payload = 0;
		data->client[i].score = 0;
		data->client[i].bush = 0;
		data->client[i].round = 0;
		// CAMP
		data->client[i].camp.x = -1;
		data->client[i].camp.y = -1;
		data->client[i].spid = 0;

		// RESOURCE MUTEX
		if (pthread_mutex_init(&data->client[i].lock, NULL)) {
			return -2;
		}
		// GLOBAL TICK SEM
		data->client[i].tick = &data->tick;
	}

	for (int i = 0; i < MAX_BEASTS; ++i) {
		data->beast[i].id = -1;
		data->beast[i].act = 0;
		data->beast[i].pos.x = -1;
		data->beast[i].pos.y = -1;
		data->beast[i].bush = 0;
		data->beast[i].see = 0;
		// RESOURCE MUTEX
		if (pthread_mutex_init(&data->beast[i].lock, NULL)) {
			return -2;
		}
	}

	// ACT
	data->act = 0;

	// RNG
	int k = 0;
	for (int i = 0; i < MAP_HEIGHT; ++i) {
		for (int j = 0; j < MAP_WIDTH; ++j) {
			if (ref_map[i][j] == L' ') {
				data->rng[k].x = j;
				data->rng[k].y = i;
				k++;
			}
		}
	}
	data->rng_len = k;

	// ENTITY
	for (int i = 0; i < MAX_ENTITY; ++i) {
		data->ent[i].type = L'q';
	}

	// DATA
	data->round = 0;
	data->pid = getpid();
	data->camp = data->rng[rand() % data->rng_len];

	return 0;
}
int server_data_destroy(struct server_data_t *data) {
	if (data == NULL) {
		return 1;
	}
	if (sem_destroy(&data->tick)) {
		return -1;
	}
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		// CLIENT OFF
		data->client[i].id = -1;
		// DEFAULT POS
		data->client[i].pos.x = -1;
		data->client[i].pos.y = -1;
		// DEFAULT ACT
		data->client[i].act = 0;
		// RESOURCE MUTEX
		if (pthread_mutex_destroy(&data->client[i].lock)) {
			return -2;
		}
		// GLOBAL TICK SEM
		data->client[i].tick = NULL;
	}

	data->round = -1;

	return 0;
}

#endif