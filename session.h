#ifndef __SESSION_H__
#define __SESSION_H__

#define SESSION_TX_NAME "_client_tx"
#define SESSION_RX_NAME "_client_rx"
#define SESSION_SH_NAME "_client_sh"

#define STRLEN(s) ((sizeof(s)/sizeof(s[0])) + 2)
#define SESSION_TX_LEN (STRLEN(SESSION_TX_NAME))
#define SESSION_RX_LEN (STRLEN(SESSION_RX_NAME))
#define SESSION_SH_LEN (STRLEN(SESSION_SH_NAME))

#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "structs.h"

struct sess_t {
	sem_t 					*tx;
	sem_t 					*rx;
	struct client_data_t 	*sh;
};

int server_session_init(struct sess_t *sess, int id) {
	if (sess == NULL || id < 0 || id > 9) {
		return 1;
	}
	char tx_name[SESSION_TX_LEN] = { 0 }; sprintf(tx_name, "%d%s", id, SESSION_TX_NAME);
	char rx_name[SESSION_RX_LEN] = { 0 }; sprintf(rx_name, "%d%s", id, SESSION_RX_NAME);
	char sh_name[SESSION_SH_LEN] = { 0 }; sprintf(sh_name, "%d%s", id, SESSION_SH_NAME);

	sess->tx = sem_open(tx_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	if (sess->tx == SEM_FAILED) {
		return -1;
	}
	sess->rx = sem_open(rx_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	if (sess->rx == SEM_FAILED) {
		sem_close(sess->tx);
		sem_unlink(tx_name);
		return -2;
	}
	int fd = shm_open(sh_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		sem_close(sess->tx);
		sem_unlink(tx_name);
		sem_close(sess->rx);
		sem_unlink(rx_name);
		return -3;
	}
	if (ftruncate(fd, sizeof(struct client_data_t)) != 0) {
		sem_close(sess->tx);
		sem_unlink(tx_name);
		sem_close(sess->rx);
		sem_unlink(rx_name);
		return -3;
	}
	sess->sh = mmap(NULL, sizeof(struct client_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sess->sh == MAP_FAILED) {
		sem_close(sess->tx);
		sem_unlink(tx_name);
		sem_close(sess->rx);
		sem_unlink(rx_name);
		return -3;
	}
	if (close(fd) != 0) {
		sem_close(sess->tx);
		sem_unlink(tx_name);
		sem_close(sess->rx);
		sem_unlink(rx_name);
		munmap(sess->sh, sizeof(struct client_data_t));
		return -3;
	}
	return 0;
}
int server_session_delete(struct sess_t *sess, int id) {
	if (sess == NULL || id < 0 || id > 9 || sess->tx == NULL || sess->rx == NULL || sess->sh == NULL) {
		return 1;
	}
	char tx_name[SESSION_TX_LEN] = { 0 }; sprintf(tx_name, "%d%s", id, SESSION_TX_NAME);
	char rx_name[SESSION_RX_LEN] = { 0 }; sprintf(rx_name, "%d%s", id, SESSION_RX_NAME);
	char sh_name[SESSION_SH_LEN] = { 0 }; sprintf(sh_name, "%d%s", id, SESSION_SH_NAME);

	if (sem_close(sess->tx) != 0 || sem_unlink(tx_name) != 0) {
		return -1;
	}
	if (sem_close(sess->rx) != 0 || sem_unlink(rx_name) != 0) {
		return -2;
	}
	if (munmap(sess->sh, sizeof(struct client_data_t)) != 0 || shm_unlink(sh_name) != 0) {
		return -3;
	}
	sess = NULL;
	return 0;
}
int client_session_init(struct sess_t *sess, int id) {
	if (sess == NULL || id < 0 || id > 9) {
		return 1;
	}
	char tx_name[SESSION_TX_LEN] = { 0 }; sprintf(tx_name, "%d%s", id, SESSION_TX_NAME);
	char rx_name[SESSION_RX_LEN] = { 0 }; sprintf(rx_name, "%d%s", id, SESSION_RX_NAME);
	char sh_name[SESSION_SH_LEN] = { 0 }; sprintf(sh_name, "%d%s", id, SESSION_SH_NAME);

	sess->tx = sem_open(rx_name, O_EXCL);
	if (sess->tx == SEM_FAILED) {
		return -1;
	}
	sess->rx = sem_open(tx_name, O_EXCL);
	if (sess->rx == SEM_FAILED) {
		sem_close(sess->tx);
		return -2;
	}
	int fd = shm_open(sh_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		sem_close(sess->tx);
		sem_close(sess->rx);
		return -3;
	}
	if (ftruncate(fd, sizeof(struct client_data_t)) != 0) {
		sem_close(sess->tx);
		sem_close(sess->rx);
		return -3;
	}
	sess->sh = mmap(NULL, sizeof(struct client_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sess->sh == MAP_FAILED) {
		sem_close(sess->tx);
		sem_close(sess->rx);
		return -3;
	}
	if (close(fd) != 0) {
		sem_close(sess->tx);
		sem_close(sess->rx);
		munmap(sess->sh, sizeof(struct client_data_t));
		return -3;
	}
	return 0;
}
int client_session_delete(struct sess_t *sess) {
	if (sess == NULL || sess->tx == NULL || sess->rx == NULL || sess->sh == NULL) {
		return 1;
	}
	if (sem_close(sess->tx) != 0) {
		return -1;
	}
	if (sem_close(sess->rx) != 0) {
		return -2;
	}
	if (munmap(sess->sh, sizeof(struct client_data_t)) != 0) {
		return -3;
	}
	sess = NULL;
	return 0;
}

#endif