#define __CLIENT__
#define _GNU_SOURCE

#include "utils.h"
#include "conn.h"
#include "session.h"
#include "structs.h"
#include <string.h>
#include <stdlib.h>

void draw(struct client_data_t *data) {
	clear();
	refresh();

	// DRAW MAP
	for (int j = 0; j < 2*FOV + 1; ++j) {
		for (int k = 0; k < 2*FOV + 1; ++k) {
			if (data->map[j][k] == L'1' || data->map[j][k] == L'2' ||
				data->map[j][k] == L'3' || data->map[j][k] == L'4') {
				attron(COLOR_PAIR(2));
			}
			else if (data->map[j][k] == L'A') {
				attron(COLOR_PAIR(5));
			}
			else if (data->map[j][k] == L'*') {
				attron(COLOR_PAIR(3));
			}
			else if (data->map[j][k] == L'D') {
				attron(COLOR_PAIR(6));
			}
			else if (data->map[j][k] == L'c' || data->map[j][k] == L't' || data->map[j][k] == L'T') {
				attron(COLOR_PAIR(4));
			} else {
				attron(COLOR_PAIR(1));
			}
			mvprintw(j,k, "%lc", data->map[j][k]);
		}
	}

	attron(COLOR_PAIR(1));
	mvprintw(1, MAP_WIDTH + 3, "Server's PID: %u", data->spid);
	if (data->camp.x == -1 || data->camp.y == -1) {
		mvprintw(2, MAP_WIDTH + 4, "Campsite X/Y: unknown");
	} else {
		mvprintw(2, MAP_WIDTH + 4, "Campsite X/Y: %02u/%02u", data->camp.x, data->camp.y);
	}
	mvprintw(1, MAP_WIDTH + 38, "Input: %s", data->act == KEY_LEFT ? "KEY_LEFT" : data->act == KEY_RIGHT ? "KEY_RIGHT" : data->act == KEY_UP ? "KEY_UP" : data->act == KEY_DOWN ? "KEY_DOWN" : "");
	mvprintw(3, MAP_WIDTH + 4, "Round number: %u", data->round);
	mvprintw(5, MAP_WIDTH + 3, "Player:");
	mvprintw(6, MAP_WIDTH + 4, "Number");
	mvprintw(7, MAP_WIDTH + 4, "Type");
	mvprintw(8, MAP_WIDTH + 4, "Curr X/Y");
	mvprintw(9, MAP_WIDTH + 4, "Deaths");
	mvprintw(11, MAP_WIDTH + 4, "Coins");
	mvprintw(12, MAP_WIDTH + 8, "carried");
	mvprintw(13, MAP_WIDTH + 8, "brought");
	attron(COLOR_PAIR(1));
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

	mvprintw(6, MAP_WIDTH + 17, "%d", data->id + 1);
	if (data->type) {
		mvprintw(7, MAP_WIDTH + 17, "CPU");
	} else {
		mvprintw(7, MAP_WIDTH + 17, "PLAYER");
	}
	
	mvprintw(8, MAP_WIDTH + 17, "%02d/%02d", data->pos.x, data->pos.y);
	mvprintw(9, MAP_WIDTH + 17, "%d", data->deaths);
	mvprintw(12, MAP_WIDTH + 17, "%d", data->payload);
	mvprintw(13, MAP_WIDTH + 17, "%d", data->score);

	refresh();
}


int main(void) {
	// REGISTER INTERRUPT
	signal(SIGINT, interr);
	// NCURSES
	if (curses_init()) {
		return 1;
	}
	// CLIENT DATA
	struct client_data_t data;
	// CONNECTION
	struct conn_t conn;
	if (client_sems_init(&conn)) {
		return show_err(__LINE__);
	}
	// INITIAL DIALOG
	sem_post(conn.cx);
	if (timed_wait(conn.rx)) {
		return show_err(__LINE__);
	}
	data.id = *conn.sh;
	if (data.id < 0) {
		return show_err(__LINE__);
	}
	sem_post(conn.tx);
	// DELETE UNUSED RESOURCES
	if (client_sems_delete(&conn)) {
		return show_err(__LINE__);
	}

	// SESSION
	struct sess_t sess;
	if (client_session_init(&sess, data.id)) {
		return show_err(__LINE__);
	}
	// KEYBOARD
	struct kbd_t kbd;
	if (keyboard_init(&kbd)) {
		client_session_delete(&sess);
		return show_err(__LINE__);
	}


	while(1) {
		// GET KEYBOARD
		data.act = keyboard_read(&kbd);
		// DISPLAY DATA
		draw(&data);

		// SAVE INFO
		sess.sh->act = data.act;
		sess.sh->pid = getpid();
		sess.sh->type = 0;
		// IF QUITS
		if (data.act == QUIT) {
			// LAST POST
			sem_post(sess.tx);
			break;
		}
		// POST INFO
		sem_post(sess.tx);

		// WAIT FOR TICK
		if (timed_wait(sess.rx)) {
			break;
		}
		// COPY DATA
		memcpy(&data, sess.sh, sizeof(struct client_data_t));
	}

	// CLEAN UP
	client_session_delete(&sess);
	keyboard_destroy(&kbd);
	curses_destroy();
	return 0;
}