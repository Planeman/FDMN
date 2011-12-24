#include "net_interface.h"

extern cSettings *_settings;
extern cLog *_log;

cNetInterface::cNetInterface() {
	initialized = false;
	return;
}

cNetInterface::cNetInterface(int port) {
	initialized = false;
	init_net( port );

	if ( INI_EXISTS(Network, max_retries) ) {
		max_retries = INI_EXTRACT(Network, max_retries, int);
	} else {
		max_retries = DEFAULT_RETRIES;
	}
	return;
}

void cNetInterface::init_net(int port) {
	if ( initialized ) {
		_log->log_simple("cNetInterface::init_net() called when already initialized");
		return;
	}

	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Failed to create network socket");
		string msg("Fatal Error: Failed to create network socket (");
		msg += strerror(errno);
		msg += ")";
		_log->log_simple(msg);

		/* Add function in server core to halt after this */
		return;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons( port );
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind(sock, (struct sockaddr*) &server_addr,
			sizeof(server_addr)) == -1) {
		perror("Failed to bind network socket");
        string msg("Fatal Error: Failed to bind network socket (");
		msg += strerror(errno);
		msg += ")";
		_log->log_simple( msg );

		/* Exit Server */
		return;
	}

	/* Enable port reuse incase the server crashes */
	int enable = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
				(char*)&enable, sizeof(int));

	if ( listen(sock, LISTEN_SIZE) == -1 ) {
		perror("Failed to set TCP listen queue");
		string msg("Failed to set TCP listen queue (");
		msg += strerror(errno);
		msg += ")";
		_log->log_simple( msg );

		/* Exit Server */
		return;
	}

	initialized = true;
	return;
}

void cNetInterface::start_listening() {
	sockaddr_in client;
	int failed_attempts = 0;

	int client_sz = sizeof(client);

    string msg;

	while ( failed_attempts <= max_retries ) {
		if ( accept(sock, (sockaddr*) &client,
					(socklen_t*) &client_sz) == -1) {

			perror("Error waiting for network connection");

			msg.clear();
			msg += "Error waiting for network connection (";
			msg += strerror(errno);
			msg += ")";

			_log->log_simple( msg );

			++failed_attempts;
			continue;
		}

		msg.clear();
		msg += "Received connection from: ";
		msg += inet_ntoa(client.sin_addr);

		_log->log_simple( msg );
	}

	return;
}
