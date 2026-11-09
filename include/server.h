#ifndef SERVER_H
#define SERVER_H

// Start the local embedded HTTP/JSON API server in a background thread
// If port is 0, an ephemeral port is automatically chosen
int server_start_background(int port);

// Get the actual port the server is listening on
int server_get_bound_port(void);

// Stop the running server and clean up sockets
void server_stop(void);

// Check if server is currently running
int server_is_running(void);

#endif // SERVER_H
