#ifndef __UTILS__
#define __UTILS__

#include "structs.h"
#include <ncurses.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#define QUIT 'q'

const int beast_actions[] = {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN};

#ifdef __CLIENT__
	const int actions[] = {QUIT, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN};
#else
	const int actions[] = {QUIT, 'c', 't', 'T', 'b'};
#endif

#define ACTION_SIZE (sizeof(actions) / sizeof(int))

#define BEAST_ACTION_SIZE (sizeof(beast_actions) / sizeof(int))


volatile sig_atomic_t flag = 0;

void interr(int sig) {
	flag = 1;
}

struct kbd_t {
	int 			act;
	pthread_t 		kbd_thread;
	pthread_mutex_t lock;
};

void *keyboard_thread(void *args) {
	int c = 0;
	while(1) {
		c = getch();
		for (int i = 0; i < ACTION_SIZE; ++i) {
			if (c == actions[i]) {
				pthread_mutex_lock(&((struct kbd_t *)args)->lock);
					((struct kbd_t *)args)->act = c;
				pthread_mutex_unlock(&((struct kbd_t *)args)->lock);
				break;
			}
		}
		if (c == QUIT) {
			break;
		}
	}
}

int keyboard_init(struct kbd_t *kbd) {
	if (kbd == NULL) {
		return 1;
	}
	if (pthread_mutex_init(&kbd->lock, NULL)) {
		return -1;
	}
	if (pthread_create(&kbd->kbd_thread, NULL, keyboard_thread, kbd)) {
		return -2;
	}
	kbd->act = 0;
	return 0;
}
int keyboard_read(struct kbd_t *kbd) {
	if (kbd == NULL) {
		return 1;
	}
	pthread_mutex_lock(&kbd->lock);
		int ret = kbd->act;
		kbd->act = 0;
	pthread_mutex_unlock(&kbd->lock);	
	return ret;
}
int keyboard_destroy(struct kbd_t *kbd) {
	if (kbd == NULL) {
		return 1;
	}
	if (pthread_cancel(kbd->kbd_thread)) {
		return -2;
	}
	if (pthread_mutex_destroy(&kbd->lock) != 0) {
		return -1;
	}
	return 0;
}
int curses_init() {
	setlocale(LC_ALL, "");
	if (initscr() == NULL) {
		return 1;
	}
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	timeout(-1);

	// DEFINE COLORS
	start_color();
	// DEFAULT
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	// PLAYER
	init_pair(2, COLOR_WHITE, COLOR_MAGENTA);
	// BEAST
	init_pair(3, COLOR_RED, COLOR_BLACK);
	// TREASURES
	init_pair(4, COLOR_BLACK, COLOR_YELLOW);
	// CAMPSITE
	init_pair(5, COLOR_YELLOW, COLOR_GREEN);
	// DROPPED TREASURE
	init_pair(6, COLOR_GREEN, COLOR_YELLOW);

	return 0;
}
int curses_destroy() {
	if (endwin()) {
		return 1;
	}	
	return 0;
}
int show_err(int line) {
	clear();
	mvprintw(0, 0, "Err no: %d in %d.\n", errno, line);
	mvprintw(1, 0, "Press any key to continue...", errno, __FILE__, line);
	refresh();
	timeout(5000);
	getch();
	endwin();
	return errno;
}
int timed_wait(sem_t *sem) {
	if (sem == NULL) {
		return 1;
	}
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += TIMEOUT;
	return sem_timedwait(sem, &timeout);
}
#endif