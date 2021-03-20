#ifndef __CONN_H__
#define __CONN_H__

#define SERVER_CX_NAME "server_cx"
#define SERVER_TX_NAME "server_tx"
#define SERVER_RX_NAME "server_rx"
#define SERVER_SH_NAME "server_sh"

#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

struct conn_t {
	sem_t *cx;
	sem_t *tx;
	sem_t *rx;
	int   *sh;
};

int server_sems_init(struct conn_t *sems) {
	if (sems == NULL) {
		return 1;
	}
	sems->cx = sem_open(SERVER_CX_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	if (sems->cx == SEM_FAILED) {
		return -1;
	}
	sems->tx = sem_open(SERVER_TX_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	if (sems->tx == SEM_FAILED) {
		sem_close(sems->cx);
		sem_unlink(SERVER_CX_NAME);
		return -2;
	}
	sems->rx = sem_open(SERVER_RX_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	if (sems->rx == SEM_FAILED) {
		sem_close(sems->cx);
		sem_unlink(SERVER_CX_NAME);
		sem_close(sems->tx);
		sem_unlink(SERVER_TX_NAME);
		return -3;
	}
	int fd = shm_open(SERVER_SH_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		sem_close(sems->cx);
		sem_unlink(SERVER_CX_NAME);
		sem_close(sems->tx);
		sem_unlink(SERVER_TX_NAME);
		sem_close(sems->rx);
		sem_unlink(SERVER_RX_NAME);
		return -4;
	}
	if (ftruncate(fd, sizeof(int)) != 0) {
		sem_close(sems->cx);
		sem_unlink(SERVER_CX_NAME);
		sem_close(sems->tx);
		sem_unlink(SERVER_TX_NAME);
		sem_close(sems->rx);
		sem_unlink(SERVER_RX_NAME);
		shm_unlink(SERVER_SH_NAME);
		return -4;
	}
	sems->sh = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sems->sh == MAP_FAILED) {
		sem_close(sems->cx);
		sem_unlink(SERVER_CX_NAME);
		sem_close(sems->tx);
		sem_unlink(SERVER_TX_NAME);
		sem_close(sems->rx);
		sem_unlink(SERVER_RX_NAME);
		shm_unlink(SERVER_SH_NAME);
		return -4;
	}
	if (close(fd) != 0) {
		sem_close(sems->cx);
		sem_unlink(SERVER_CX_NAME);
		sem_close(sems->tx);
		sem_unlink(SERVER_TX_NAME);
		sem_close(sems->rx);
		sem_unlink(SERVER_RX_NAME);
		shm_unlink(SERVER_SH_NAME);
		munmap(sems->sh, sizeof(int));
		return -4;
	}
	return 0;
}
int server_sems_delete(struct conn_t *sems) {
	if (sems == NULL || sems->cx == NULL || sems->tx == NULL || sems->rx == NULL || sems->sh == NULL) {
		return 1;
	}
	if (sem_close(sems->cx) != 0 || sem_unlink(SERVER_CX_NAME) != 0) {
		return -1;
	}
	if (sem_close(sems->tx) != 0 || sem_unlink(SERVER_TX_NAME) != 0) {
		return -2;
	}
	if (sem_close(sems->rx) != 0 || sem_unlink(SERVER_RX_NAME) != 0) {
		return -3;
	}
	if (munmap(sems->sh, sizeof(int)) != 0 || shm_unlink(SERVER_SH_NAME) != 0) {
		return -4;
	}
	sems = NULL;
	return 0;
}
int client_sems_init(struct conn_t *sems) {
	if (sems == NULL) {
		return 1;
	}
	sems->cx = sem_open(SERVER_CX_NAME, O_EXCL);
	if (sems->cx == SEM_FAILED) {
		return -1;
	}
	sems->tx = sem_open(SERVER_RX_NAME, O_EXCL);
	if (sems->tx == SEM_FAILED) {
		sem_close(sems->cx);
		return -2;
	}
	sems->rx = sem_open(SERVER_TX_NAME, O_EXCL);
	if (sems->rx == SEM_FAILED) {
		sem_close(sems->cx);
		sem_close(sems->tx);
		return -3;
	}
	int fd = shm_open(SERVER_SH_NAME, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		sem_close(sems->cx);
		sem_close(sems->tx);
		sem_close(sems->rx);
		return -4;
	}
	if (ftruncate(fd, sizeof(int)) != 0) {
		sem_close(sems->cx);
		sem_close(sems->tx);
		sem_close(sems->rx);
		return -4;
	}
	sems->sh = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sems->sh == MAP_FAILED) {
		sem_close(sems->cx);
		sem_close(sems->tx);
		sem_close(sems->rx);
		return -4;
	}
	if (close(fd) != 0) {
		sem_close(sems->cx);
		sem_close(sems->tx);
		sem_close(sems->rx);
		munmap(sems->sh, sizeof(int));
		return -4;
	}
	return 0;
}
int client_sems_delete(struct conn_t *sems) {
	if (sems == NULL || sems->cx == NULL || sems->tx == NULL || sems->rx == NULL || sems->sh == NULL) {
		return 1;
	}
	if (sem_close(sems->cx) != 0) {
		return -1;
	}
	if (sem_close(sems->rx) != 0) {
		return -2;
	}
	if (sem_close(sems->tx) != 0) {
		return -3;
	}
	if (munmap(sems->sh, sizeof(int)) != 0) {
		return -4;
	}
	sems = NULL;
	return 0;
}

#endif