#include <commons/string.h>
#include <pthread.h>

#include "../../libs/mocks/client_server.h"
#include "../coordinator_api.h"
#include "../entry_table.h"
#include "../instancia.h"
#include "../storage.h"
#include "coordinador_mock.h"

int		total_entries, entry_size, key_amount;
char	**keys, **values;
t_list	*recoverable_keys;

void client_server_execute_server(int fd_client) {
	bool is_accepted = key_amount > 1;

	coordinador_mock_handshake(fd_client, is_accepted, total_entries, entry_size, recoverable_keys);

	if(!is_accepted) {
		pthread_exit(NULL);
	}

	int i, status;

	for(i = 1; i < key_amount; i++) {
		coordinador_mock_set_request(fd_client, keys[i - 1], values[i]);

		status = coordinador_mock_set_response(fd_client);

		if(status == 0) {
			messenger_show("ERROR", "Ocurrio un error en la Instancia cuando recibio el pedido de SET");

			break;
		}

		coordinador_mock_store_request(fd_client, keys[i - 1]);

		status = coordinador_mock_store_response(fd_client);

		if(status == 0) {
			messenger_show("ERROR", "Ocurrio un error en la Instancia cuando recibio el pedido de STORE");

			break;
		}
	}

	coordinador_mock_kill(fd_client);
}

void client_server_execute_client(int fd_server) {
	is_closable_client = false;

	fd_coordinador = fd_server;

	int status = instance_init("Instancia", "../instancia.log", "INFO", "../instancia.cfg");

	if(status == INSTANCE_INIT_ERROR) {
		pthread_exit(NULL);
	}

	instance_main();

	instance_die();
}

int main(int argc, char* argv[]) {
	server_name = "Coordinador";
	client_name = "Instancia 1";

	total_entries = 8;
	entry_size = 4;
	key_amount = argc;

	keys = calloc(key_amount, sizeof(char*));
	values = argv;

	recoverable_keys = list_create();

	int i;
	for(i = 1; i < key_amount; i++) {
		keys[i - 1] = string_from_format("clave_%d", i);

		list_add(recoverable_keys, keys[i - 1]);
	}

	client_server_run();

	list_destroy_and_destroy_elements(recoverable_keys, free);

	free(keys);
}
