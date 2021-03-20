#define _GNU_SOURCE

#include "structs.h"
#include "conn.h"
#include "utils.h"
#include "session.h"
#include <string.h>
#include <signal.h>

#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#include <time.h>
#include <stdlib.h>


void move_c(struct client_data_t *client) {
	if (client != NULL && client->pos.x >= 0 && client->pos.y >= 0 && client->act != 0) {
		if (client->bush) {
			client->bush = 0;
			return;
		}
		switch(client->act) {
			case KEY_LEFT:
				if (ref_map[client->pos.y][client->pos.x - 1] != L'█') {
					client->pos.x -= 1;
				}
				break;
			case KEY_RIGHT:
				if (ref_map[client->pos.y][client->pos.x + 1] != L'█') {
					client->pos.x += 1;
				}
				break;
			case KEY_UP:
				if (ref_map[client->pos.y - 1][client->pos.x] != L'█') {
					client->pos.y -= 1;
				}
				break;
			case KEY_DOWN:
				if (ref_map[client->pos.y + 1][client->pos.x] != L'█') {
					client->pos.y += 1;
				}
				break;
		}
		if (ref_map[client->pos.y][client->pos.x] == L'#') {
			client->bush = 1;
			return;
		}
	}
}
void *client_thread(void *args) {
	struct client_data_t *data = (struct client_data_t *)args;
	struct sess_t sess;
	int ID = data->id;

	if (server_session_init(&sess, ID)) {
		return NULL;
	}

	// SIG AFTER CREATING RESOURCES
	pthread_mutex_unlock(&data->lock);

	while(1) {
		// WAIT FOR CLIENT
		if (timed_wait(sess.rx) || flag) {
			break;
		}
		// COPY ONLY MOVEMENT & PID
		pthread_mutex_lock(&data->lock);
		data->act = sess.sh->act;
		data->pid = sess.sh->pid;
		pthread_mutex_unlock(&data->lock);
		// IF QUIT
		if (sess.sh->act == QUIT) {
			break;
		}
		// WAIT FOR MAIN LOOP TICK
		if (timed_wait(data->tick) || flag) {
			break;
		}
		// COPY FULL STRUCT
		pthread_mutex_lock(&data->lock);
		memcpy(sess.sh, data, sizeof(struct client_data_t));
		pthread_mutex_unlock(&data->lock);
		// POST UPDATE
		sem_post(sess.tx);
	}

	// FINISH ROUTINE
	pthread_mutex_lock(&data->lock);
	data->id = -1;
	data->act = 0;
	data->pos.x = -1;
	data->pos.y = -1;
	data->pid = -1;
	data->deaths = 0;
	data->score = 0;
	data->payload = 0;
	data->camp.x = -1;
	data->camp.y = -1;
	data->spid = 0;
	pthread_mutex_unlock(&data->lock);
	server_session_delete(&sess, ID);
}
void move_b(struct beast_data_t *client) {
	if (client != NULL && client->pos.x >= 0 && client->pos.y >= 0 && client->act != 0) {
		if (client->bush) {
			client->bush = 0;
			return;
		}
		switch(client->act) {
			case KEY_LEFT:
				if (ref_map[client->pos.y][client->pos.x - 1] != L'█') {
					client->pos.x -= 1;
				}
				break;
			case KEY_RIGHT:
				if (ref_map[client->pos.y][client->pos.x + 1] != L'█') {
					client->pos.x += 1;
				}
				break;
			case KEY_UP:
				if (ref_map[client->pos.y - 1][client->pos.x] != L'█') {
					client->pos.y -= 1;
				}
				break;
			case KEY_DOWN:
				if (ref_map[client->pos.y + 1][client->pos.x] != L'█') {
					client->pos.y += 1;
				}
				break;
		}
		if (ref_map[client->pos.y][client->pos.x] == L'#') {
			client->bush = 1;
			return;
		}
	}
}
void *beast_thread(void *args) {
	struct beast_data_t *data = (struct beast_data_t *)args;

	while(1) {
		pthread_mutex_lock(&data->lock);

		// RAY SCAN
		data->see = 0;
		// NORTH
		for (int i = 0; i < FOV; ++i) {
			if (data->map[FOV - i][FOV] == L'█') {
				break;
			}
			if (iswdigit(data->map[FOV - i][FOV])) {
				data->act = KEY_UP;
				data->see = 1;
				break;
			}
		}
		// SOUTH
		for (int i = 0; i < FOV; ++i) {
			if (data->map[FOV + i][FOV] == L'█') {
				break;
			}
			if (iswdigit(data->map[FOV + i][FOV])) {
				data->act = KEY_DOWN;
				data->see = 1;
				break;
			}
		}
		// WEST
		for (int i = 0; i < FOV; ++i) {
			if (data->map[FOV][FOV - i] == L'█') {
				break;
			}
			if (iswdigit(data->map[FOV][FOV - i])) {
				data->act = KEY_LEFT;
				data->see = 1;
				break;
			}
		}
		// EAST
		for (int i = 0; i < FOV; ++i) {
			if (data->map[FOV][FOV + i] == L'█') {
				break;
			}
			if (iswdigit(data->map[FOV][FOV + i])) {
				data->act = KEY_RIGHT;
				data->see = 1;
				break;
			}
		}
		if (!data->see) {
			data->act = beast_actions[(rand() % BEAST_ACTION_SIZE)];
		}
	}
	// FINISH ROUTINE
	data->id = -1;
	data->act = 0;
}

void spawn_e(struct server_data_t *data) {
	if (data != NULL) {
		struct entity_t tmp;
		tmp.type = data->act;
		tmp.pos = data->rng[rand() % data->rng_len];
		switch(data->act) {
			case L'c':
			case L't':
			case L'T':
				for (int i = 0; i < MAX_ENTITY; ++i) {
					if (data->ent[i].type == L'q') {
						data->ent[i] = tmp;
						break;
					}
				}
				break;
			case L'b':
			case L'B':
				// FIND SPACE
				for (int i = 0; i < MAX_BEASTS; ++i) {
					if (data->beast[i].id < 0) {
						data->beast[i].id = i;
						pthread_mutex_lock(&data->beast[i].lock);
						if (pthread_create(&data->beast_thread[i], NULL, beast_thread, &data->beast[i])) {
							data->beast[i].id = -1;
						}
						break;
					}
				}
				break;
		}
	}
}

void update(struct server_data_t *data) {
	// SPAWN NEW ENTITIES (INCLUDING BEASTS)
	spawn_e(data);
	// UPDATE CLIENTS
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (data->client[i].id >= 0) {
			// INIT NEW CLIENTS
			if (data->client[i].pos.x < 0 && data->client[i].pos.y < 0) {
				// RAND NEW COORDS
				data->client[i].pos = data->rng[rand() % data->rng_len];
				continue;
			}
			// HANDLE MOVEMENT
			move_c(&data->client[i]);
			
			// HANDLE COLLISION WITH ENTITIES
			for (int j = 0; j < MAX_ENTITY; ++j) {
				if (data->ent[j].type != L'q' && !point_cmp(data->ent[j].pos, data->client[i].pos)) {
					// ADD TO BANK
					switch (data->ent[j].type) {
						case L'c':
							data->client[i].payload += 5;
							break;
						case L't':
							data->client[i].payload += 10;
							break;
						case L'T':
							data->client[i].payload += 50;
							break;
						case L'D':
							data->client[i].payload += data->ent[j].value;
							break;
					}

					data->ent[j].type = L'q';
				}
			}
			// HANDLE COLLISION WITH CAMPSITE
			if (!point_cmp(data->client[i].pos, data->camp)) {
				data->client[i].score += data->client[i].payload;
				data->client[i].payload = 0;
				if (data->client[i].camp.x == -1 && data->client[i].camp.y == -1) {
					data->client[i].camp = data->camp;
				}
			}

			// HANDLE COLLISION WITH PLAYERS
			for (int j = 0; j < MAX_CLIENTS; ++j) {
				if (i != j && !point_cmp(data->client[i].pos, data->client[j].pos)) {
					for (int k = 0; k < MAX_ENTITY; ++k) {
						if (data->ent[k].type == L'q') {
							data->ent[k].type = 'D';
							data->ent[k].value = data->client[i].payload + data->client[j].payload;
							data->ent[k].pos.x = data->client[i].pos.x;
							data->ent[k].pos.y = data->client[i].pos.y;
							break;
						}
					}
					data->client[i].pos.x = -1;
					data->client[i].pos.y = -1;
					data->client[i].payload = 0;
					data->client[i].deaths += 1;

					data->client[j].pos.x = -1;
					data->client[j].pos.y = -1;
					data->client[j].payload = 0;
					data->client[j].deaths += 1;
					break;
				}
			}
			// HANDLE COLLISION WITH BEASTS
			for (int j = 0; j < MAX_BEASTS; ++j) {
				if (data->beast[j].pos.x >= 0 && data->beast[j].pos.y >= 0) {
					if (!point_cmp(data->client[i].pos, data->beast[j].pos)) {
						for (int k = 0; k < MAX_ENTITY; ++k) {
							if (data->ent[k].type == L'q') {
								data->ent[k].type = 'D';
								data->ent[k].value = data->client[i].payload;
								data->ent[k].pos.x = data->client[i].pos.x;
								data->ent[k].pos.y = data->client[i].pos.y;
								break;
							}
						}
						data->client[i].pos.x = -1;
						data->client[i].pos.y = -1;
						data->client[i].payload = 0;
						data->client[i].deaths += 1;
						break;
					}
				}
			}
		}
	}
	// UPDATE BEASTS
	for (int i = 0; i < MAX_BEASTS; ++i) {
		if (data->beast[i].pos.x < 0 && data->beast[i].pos.y < 0) {
			// RAND NEW COORDS
			data->beast[i].pos = data->rng[rand() % data->rng_len];
			continue;
		}
		if (data->beast[i].id >= 0) {
			// UPDATE MOVEMENT
			move_b(&data->beast[i]);
		}
	}
}

void draw(struct server_data_t *data) {
	clear();
	refresh();

	// DRAW LEGEND
	attron(COLOR_PAIR(1));
	mvprintw(1, MAP_WIDTH + 38, "Input: %lc", data->act);
	mvprintw(1, MAP_WIDTH + 3, "Server's PID: %u", data->pid);
	mvprintw(2, MAP_WIDTH + 4, "Campsite X/Y: %02u/%02u", data->camp.x, data->camp.y);
	mvprintw(3, MAP_WIDTH + 4, "Round number: %u", data->round);
	mvprintw(5, MAP_WIDTH + 3, "Parameter:");
	mvprintw(6, MAP_WIDTH + 4, "PID");
	mvprintw(7, MAP_WIDTH + 4, "Type");
	mvprintw(8, MAP_WIDTH + 4, "Curr X/Y");
	mvprintw(9, MAP_WIDTH + 4, "Deaths");
	mvprintw(11, MAP_WIDTH + 4, "Coins");
	mvprintw(12, MAP_WIDTH + 8, "carried");
	mvprintw(13, MAP_WIDTH + 8, "brought");
	mvprintw(16, MAP_WIDTH + 3, "Legend:");
	mvprintw(17, MAP_WIDTH + 8, " – players");
	mvprintw(18, MAP_WIDTH + 4, "█    – wall");
	mvprintw(19, MAP_WIDTH + 4, "#    – bushes (slow down)");
	mvprintw(20, MAP_WIDTH + 8, " – beast");
	mvprintw(21, MAP_WIDTH + 8, " – one coin");
	mvprintw(22, MAP_WIDTH + 8, " – treasure (10 coins)");
	mvprintw(23, MAP_WIDTH + 8, " – large treasure (50 coins)");
	mvprintw(24, MAP_WIDTH + 8, " – campsite");
	mvprintw(21, MAP_WIDTH + 38, " – dropped treasure");
	attron(COLOR_PAIR(2));
	mvprintw(17, MAP_WIDTH + 4, "1234");
	attron(COLOR_PAIR(3));
	mvprintw(20, MAP_WIDTH + 4, "*");
	attron(COLOR_PAIR(4));
	mvprintw(21, MAP_WIDTH + 4, "c");
	mvprintw(22, MAP_WIDTH + 4, "t");
	mvprintw(23, MAP_WIDTH + 4, "T");
	attron(COLOR_PAIR(5));
	mvprintw(24, MAP_WIDTH + 4, "A");
	attron(COLOR_PAIR(6));
	mvprintw(21, MAP_WIDTH + 37, "D");
	attron(COLOR_PAIR(1));

	// DRAW MAP
	attron(COLOR_PAIR(1));
	for (int i = 0; i < MAP_HEIGHT + 1; ++i) {
		for (int j = 0; j < MAP_WIDTH + 1; ++j) {
			mvprintw(i, j, "%lc", ref_map[i][j]);
		}
	}

	// DRAW CAMP
	attron(COLOR_PAIR(5));
	mvprintw(data->camp.y, data->camp.x, "A");

	// DRAW PLAYERS
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (data->client[i].id >= 0 && data->client[i].pos.x >= 0 && data->client[i].pos.y >= 0) {
			// DRAW PLAYER
			attron(COLOR_PAIR(2));
			mvprintw(data->client[i].pos.y, data->client[i].pos.x, "%d", i + 1);
			// DRAW STATS
			attron(COLOR_PAIR(1));
			mvprintw(5, MAP_WIDTH + 17 + i*10, "Player%d", i + 1);
			mvprintw(6, MAP_WIDTH + 17 + i*10, "%d", data->client[i].pid);

			if (data->client[i].type) {
				mvprintw(7, MAP_WIDTH + 17 + i*10, "CPU");
			} else {
				mvprintw(7, MAP_WIDTH + 17 + i*10, "PLAYER");
			}
			
			

			mvprintw(8, MAP_WIDTH + 17 + i*10, "%02d/%02d", data->client[i].pos.x, data->client[i].pos.y);
			mvprintw(9, MAP_WIDTH + 17 + i*10, "%d", data->client[i].deaths);
			mvprintw(12, MAP_WIDTH + 17 + i*10, "%d", data->client[i].payload);
			mvprintw(13, MAP_WIDTH + 17 + i*10, "%d", data->client[i].score);
		}
	}
	// DRAW BEASTS
	for (int i = 0; i < MAX_BEASTS; ++i) {
		if (data->beast[i].id >= 0 && data->beast[i].pos.x >= 0 && data->beast[i].pos.y >= 0) {
			// DRAW PLAYER
			attron(COLOR_PAIR(3));
			mvprintw(data->beast[i].pos.y, data->beast[i].pos.x, "*");

			attron(COLOR_PAIR(1));
			// ID
			mvprintw(MAP_HEIGHT + 1, ((2*FOV)+7)*i, "Beast %d", i + 1);
			// STATUS
			mvprintw(MAP_HEIGHT + 2, ((2*FOV)+7)*i, "See: %s", data->beast[i].see ? "Yes" : "No");
			// ACTION
			mvprintw(MAP_HEIGHT + 3, ((2*FOV)+7)*i, "Act: %s", data->beast[i].act == KEY_LEFT ? "KEY_LEFT" : data->beast[i].act == KEY_RIGHT ? "KEY_RIGHT" : data->beast[i].act == KEY_UP ? "KEY_UP" : data->beast[i].act == KEY_DOWN ? "KEY_DOWN" : "");
			
			// DRAW MAP
			for (int j = 0; j < FOV * 2 + 1; ++j) {
				for (int k = 0; k < FOV * 2 + 1; ++k) {
					mvprintw(MAP_HEIGHT + 5 + j, ((2*FOV)+7)*i + 2 + k, "%lc", data->beast[i].map[j][k]);
				}
			}
			
		}
	}

	// DRAW ENTITIES
	for (int i = 0; i < MAX_ENTITY; ++i) {
		if (data->ent[i].type != 'q') {
			if (data->ent[i].type == 'D') {
				attron(COLOR_PAIR(6));
			} else {
				attron(COLOR_PAIR(4));
			}
			mvprintw(data->ent[i].pos.y, data->ent[i].pos.x, "%c", data->ent[i].type);
		}
	}
	refresh();

	// SHARE WITH CLIENTS
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (data->client[i].id >= 0 && data->client[i].pos.x >= 0 && data->client[i].pos.y >= 0) {
			// SHARE MAP
			for (int j = 0; j < 2*FOV + 1; ++j) {
				for (int k = 0; k < 2*FOV + 1; ++k) {
					if (data->client[i].pos.x - FOV + k < 0         ||
						data->client[i].pos.y - FOV + j < 0         ||
						data->client[i].pos.x - FOV + k > MAP_WIDTH || 
						data->client[i].pos.y - FOV + j > MAP_HEIGHT) {
						data->client[i].map[j][k] = L' ';
					} else {
						mvinnwstr(data->client[i].pos.y - FOV + j, data->client[i].pos.x - FOV + k, &data->client[i].map[j][k], 1);
					}
				}
			}
			// SHARE DATA
			data->client[i].spid = data->pid;
			data->client[i].round = data->round;
		}
	}
	// SHARE WITH BEASTS
	for (int i = 0; i < MAX_BEASTS; ++i) {
		if (data->beast[i].id >= 0 && data->beast[i].pos.x >= 0 && data->beast[i].pos.y >= 0) {
			// SHARE MAP
			for (int j = 0; j < 2*FOV + 1; ++j) {
				for (int k = 0; k < 2*FOV + 1; ++k) {
					if (data->beast[i].pos.x - FOV + k < 0         ||
						data->beast[i].pos.y - FOV + j < 0         ||
						data->beast[i].pos.x - FOV + k > MAP_WIDTH || 
						data->beast[i].pos.y - FOV + j > MAP_HEIGHT) {
						data->beast[i].map[j][k] = L' ';
					} else {
						mvinnwstr(data->beast[i].pos.y - FOV + j, data->beast[i].pos.x - FOV + k, &data->beast[i].map[j][k], 1);
					}
				}
			}
		}
	}
}

int main(void) {
	// RAND
	srand(time(NULL));
	// REGISTER INTERRUPT
	signal(SIGINT, interr);
	// NCURSES
	if (curses_init()) {
		return 1;
	}
	// SERVER DATA
	struct server_data_t data;
	if (server_data_init(&data)) {
		curses_destroy();
		return show_err(__LINE__);
	}
	// CONNECTION
	struct conn_t conn;
	if (server_sems_init(&conn)) {
		server_data_destroy(&data);
		curses_destroy();
		return show_err(__LINE__);
	}
	// KEYBOARD
	struct kbd_t kbd;
	if (keyboard_init(&kbd)) {
		server_data_destroy(&data);
		curses_destroy();
		server_sems_delete(&conn);
		return show_err(__LINE__);
	}
	while(1) {
		// HANDLE NEW CONNECTION
		if (!sem_trywait(conn.cx)) {
			// FIND SPACE
			int i = 0;
			for (; i < MAX_CLIENTS; ++i) {
				pthread_mutex_lock(&data.client[i].lock);
				if (data.client[i].id < 0) {
					data.client[i].id = i;
					*(conn.sh) = i;
					pthread_mutex_unlock(&data.client[i].lock);
					break;
				} else {
					*(conn.sh) = -1;
				}
				pthread_mutex_unlock(&data.client[i].lock);
			}
			// TODO: READ TYPE OF MACHINE
			if (*(conn.sh) >= 0) {
				// LOCK TO WAIT FOR RESOURCES
				pthread_mutex_lock(&data.client[i].lock);

				// CREATE NEW THREAD
				if (pthread_create(&data.client_thread[i], NULL, client_thread, &data.client[i])) {
					server_data_destroy(&data);
					keyboard_destroy(&kbd);
					server_sems_delete(&conn);
					curses_destroy();
					return show_err(__LINE__);
				}

				// WAIT FOR RESOURCES
				pthread_mutex_lock(&data.client[i].lock);
				pthread_mutex_unlock(&data.client[i].lock);

				// DOUBLE HANDSHAKE
				sem_post(conn.tx);
				if (timed_wait(conn.rx)) {
					pthread_mutex_lock(&data.client[i].lock);
					data.client[i].id = -1;
					pthread_mutex_unlock(&data.client[i].lock);
				}
			} else {
				sem_post(conn.tx);
			}
		}

		// HANDLE QUIT MSG
		data.act = keyboard_read(&kbd);
		if (data.act == QUIT || flag) {
			break;
		}
		
		// STANDARD CALLS
		update(&data);
		draw(&data);

		// GLOBAL TICK
		for (int i = 0; i < MAX_CLIENTS; ++i) {
			pthread_mutex_lock(&data.client[i].lock);
			if (data.client[i].id >= 0) {
				sem_post(&data.tick);
			}
			pthread_mutex_unlock(&data.client[i].lock);
		}
		for (int i = 0; i < MAX_BEASTS; ++i) {
			if (data.beast[i].id >= 0) {
				pthread_mutex_unlock(&data.beast[i].lock);
			}
		}

		// NEXT ROUND
		usleep(TICKRATE);
		data.round++;
	}

	// WAIT FOR ALL THREADS (SAFE CLOSE)
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		pthread_join(data.client_thread[i], NULL);
	}
	// CLOSE ALL BEASTS (NO RESOURCES CREATED)
	for (int i = 0; i < MAX_BEASTS; ++i) {
		pthread_cancel(data.beast_thread[i]);
	}

	// CLEAN UP
	server_data_destroy(&data);
	keyboard_destroy(&kbd);
	server_sems_delete(&conn);
	curses_destroy();
	return 0;
}