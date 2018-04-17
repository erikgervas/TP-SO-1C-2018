#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>

#include "../libs/conector.h"
#include "planificador.h"

#define MAXCONN 20

t_log* logger;
t_list* locked_keys;
t_list* esi_bursts;

int main(void) {

	logger = log_create("planificador.log", "planificador", 1 , LOG_LEVEL_INFO);

	/* TODO Obtengo la ip y el puerto del coordinador del archivo de configuracion*/
	char* host;
	int port_coordinator;
	int server_port;

	int coordinator_fd = connect_to_server(host, port_coordinator);
	if (send_handshake(coordinator_fd, SCHEDULER) != 1) {
		log_error(logger, "Failure in send_handshake");
		close(coordinator_fd);
	}

	bool confirmation;
	int received = receive_confirmation(coordinator_fd, &confirmation);
	if (!received || !confirmation) {
		log_error(logger, "Failure in confirmation reception");
		close(coordinator_fd);
	}

	int listener = init_listener(server_port, MAXCONN);

	fd_set connected_fds;
	fd_set read_fds;

	FD_ZERO(&connected_fds);
	FD_ZERO(&read_fds);
	FD_SET(coordinator_fd, &connected_fds);
	FD_SET(listener, &connected_fds);

	int max_fd = listener;

	while(1)
	{
		read_fds = connected_fds;

		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
					log_error(logger, "Select error");
					exit(EXIT_FAILURE);
		}

		int fd;

		for (fd = 0; fd <= max_fd; fd++) {

					if (FD_ISSET(fd, &read_fds) == 0) {
						continue;

					} else if (fd == listener) {

						struct sockaddr_in client_info;
						socklen_t addrlen = sizeof client_info;
						log_info(logger, "New client connecting...");

						int new_client_fd = accept(listener, (struct sockaddr *) &client_info, &addrlen);

						if (new_client_fd == -1) {

							log_error(logger, "Accept error");
						} else {

							FD_SET(new_client_fd, &connected_fds);

							if (new_client_fd > max_fd) {

								max_fd = new_client_fd;
							}

							log_info(logger, "Socket %d connected", new_client_fd);

							bool client_confirmation = false;
							if (receive_handshake(new_client_fd) == -1) {

								send_confirmation(new_client_fd, confirmation);
								remove_fd(new_client_fd, &connected_fds);
								log_error(logger, "Handshake fail with socket %d", new_client_fd);
								close(new_client_fd);
							} else {

								client_confirmation = true;
								send_confirmation(new_client_fd, confirmation);
							}

						}

					} else if(fd == coordinator_fd){

						/* TODO -- El coordinador se comunica con el planificador*/

					}
				}

	}

	return EXIT_SUCCESS;
}

void remove_fd(int fd, fd_set *fdset)
{
	FD_CLR(fd, fdset);
	log_info(logger, "Socket %d kicked out", fd);
	close(fd);
}

key_blocker* create_key_blocker(char* key, int esi_id)
{
    key_blocker* key_blocker = malloc(sizeof(key_blocker));
    key_blocker->key = key;
    key_blocker->esi_id = esi_id;
    return key_blocker;
}

void destroy_key_blocker(key_blocker* key_blocker)
{
	free(key_blocker->key);
	free(key_blocker);
}

esi_information* create_esi_information(int esi_id)
{
	esi_information* esi_inf = malloc(sizeof(esi_information));
	esi_inf->esi_id = esi_id;
	/*TODO esi_inf->next_burst = ESTIMACION_INICIAL; Se saca del archivo de config */
	esi_inf->last_burst = 0;
	return esi_inf;
}

void destroy_esi_information(esi_information* esi_inf)
{
    free(esi_inf);
}

void create_administrative_structures()
{
	locked_keys = list_create();
	esi_bursts = list_create();
}

void destroy_administrative_structures()
{
	list_destroy_and_destroy_elements(locked_keys, destroy_key_blocker);
	free(locked_keys);
	list_destroy_and_destroy_elements(esi_bursts, destroy_esi_information);
	free(esi_bursts);
}
