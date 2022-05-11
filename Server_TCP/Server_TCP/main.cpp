#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <WS2tcpip.h> //the api that windows uses to access network sockets
#include <map>

#define BUFLEN		4096

#pragma comment (lib,"ws2_32.lib")

 
int main() {

	//Initialize winsock
	WSADATA wsData;
	char buffer[BUFLEN];
	WORD ver = MAKEWORD(2, 2);
	
	int wsOK = WSAStartup(ver, &wsData);
	if (wsOK != 0) {
		std::cout << "Can't initialize winsock!\n ";
		return -1;
	}

	//Create a socket
	SOCKET listening_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (listening_sock == INVALID_SOCKET) {
		std::cout << ("Error while creating socket\n");
		return -1;
	}

	std::cout << "Socket created successfully\n";

	//Bind the ip adress and port to a socket
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(54000); //Networking is a bigEndian and PCs are littleEndian 
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY; //Could also use inet_pton ...
	
	if (bind(listening_sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		std::cout << "Couldn't bind to the port\n";
		return -1;
	}

	std::cout << "Done with binding\n"; 

	//Tell Winsock the socket is for listening
	if ( listen(listening_sock, SOMAXCONN) < 0) {
		std::cout << "Error while listening\n";
		return -1;
	}

	std::cout << "\nListening for incoming connections.....\n"; 

	fd_set master_read, copy;

	FD_ZERO(&master_read);
	FD_ZERO(&copy);

	FD_SET(listening_sock, &master_read);

	bool running = true;
	std::map <SOCKET, std::string> users;
	while (running) {

		copy = master_read; 

		int socket_count = select(0, &copy, NULL, NULL, NULL);

		for (int i = 0; i < socket_count; i++) {
			if (copy.fd_array[i] == listening_sock) {

				sockaddr_in clientt;
				int addrlen = sizeof(clientt);
				SOCKET client = accept(listening_sock, (struct sockaddr*)&clientt, &addrlen);

				FD_SET(client, &master_read);

				int clientSize = sizeof(clientt);


				char host[NI_MAXHOST];		// Client's remote name
				char service[NI_MAXSERV];	// Service (i.e. port) the client is connect on

				ZeroMemory(host, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
				ZeroMemory(service, NI_MAXSERV);

				if (getnameinfo((sockaddr*)&clientt, sizeof(clientt), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
				{
					std::cout << host << " connected on port " << service << std::endl;
				
				}
				else
				{
					inet_ntop(AF_INET, &clientt.sin_addr, host, NI_MAXHOST);
					//std::cout << host << " connected on port " <<
						//ntohs(clientt.sin_port) << std::endl;
					printf("IP address is: %s\n", inet_ntoa(clientt.sin_addr));
					printf("port is: %d\n", (int)ntohs(clientt.sin_port));
					
				}



				std::string msg  = "SERVER:Welcome to the Chat Server!";
				send(client, msg.c_str(), msg.size() + 1, 0);
				//Notify other users about the new connection
				//for (u_int j = 0; j < copy.fd_count; j++) {
				//	SOCKET out_sock = copy.fd_array[j];
				//
				//	std::ostringstream ss;
				//
				//	if (users.find(copy.fd_array[i]) != users.end()) {
				//		ss << "Say Hi to " << users[copy.fd_array[i]] << "\r\n";
				//		//std::cout << users[copy.fd_array[i]] << " (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
				//	}
				//	else {
				//		ss << "Say Hi to ANONYM (SOCKET #" << copy.fd_array[i] << ")" << "\r\n";
				//		//std::cout << "ANONYM (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
				//	}
				//	std::string str_out = ss.str();
				//	send(out_sock, str_out.c_str(), str_out.size() + 1, 0);
				//}
					
			}
			else {
				memset(buffer, 0, BUFLEN);
				int bytes_in = recv(copy.fd_array[i], buffer, BUFLEN, 0);

				std::cout << buffer << "  Mesaje received  " << i << std::endl;

				if (bytes_in <= 0) {
					//Drop the client 
					closesocket(copy.fd_array[i]);
					FD_CLR(copy.fd_array[i], &master_read);
				}
				else {
					// Check to see if it's a command. \quit kills the server
					if (buffer[0] == '\\') {
						// Is the command quit?
						std::string cmd = std::string(buffer, bytes_in);
						if (cmd == "\\quit") {
							running = false;
							break;
						}
						else if (cmd.find("\\login") != std::string::npos) { // login cmd found
							if (users.find(copy.fd_array[i]) != users.end()) { //user is already logged in
								std::string message = "You are already logged in. To change your name please use \\change.";
								send(copy.fd_array[i], message.c_str(), message.size() + 1, 0);
							}
							std::string userName = std::string(cmd.begin() + 8, cmd.end());
							users.insert(std::pair<SOCKET, std::string>(copy.fd_array[i], userName));
						}
						else if (cmd.find("\\change") != std::string::npos) {
							std::string userName = std::string(cmd.begin() + 9, cmd.end());
							users[copy.fd_array[i]] = userName;
						}
						else if (cmd.find("\\exit") != std::string::npos) {
							std::string message = "@exit";
							send(copy.fd_array[i], message.c_str(), message.size() + 1, 0);
							for (u_int j = 0; j < master_read.fd_count; j++) {
								SOCKET out_sock = master_read.fd_array[j];

								std::ostringstream ss;

								if (out_sock != listening_sock) {
									if (users.find(copy.fd_array[i]) != users.end()) {
										ss << users[copy.fd_array[i]] << " left.\r\n";
										//std::cout << users[copy.fd_array[i]] << " (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
									}
									else {
										ss << "ANONYM (SOCKET #" << copy.fd_array[i] << ") left." << "\r\n";
										//std::cout << "ANONYM (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
									}
									std::string str_out = ss.str();
									send(out_sock, str_out.c_str(), str_out.size() + 1, 0);

								}

							}
							auto it = users.find(copy.fd_array[i]);
							if (it != users.end()) {
								users.erase(it);
							}
							FD_CLR(copy.fd_array[i], &master_read);
							closesocket(copy.fd_array[i]);
							
						}
						else {
							std::string str_out = "Incorrect Command!";
							send(copy.fd_array[i], str_out.c_str(), str_out.size() + 1, 0);
						}
						continue;
				    }

				// Send message to other clients, and definiately NOT the listening socket

					for (u_int j = 0; j < master_read.fd_count; j++) {
						SOCKET out_sock = master_read.fd_array[j];

						if (out_sock == listening_sock) {
							continue;
						}
						std::ostringstream ss;

						if (out_sock != copy.fd_array[i]) {
							if (users.find(copy.fd_array[i]) != users.end()) {
								ss << users[copy.fd_array[i]] << " (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
								std::cout << users[copy.fd_array[i]] << " (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
							}
							else {
								ss << "ANONYM (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
								std::cout << "ANONYM (SOCKET #" << copy.fd_array[i] << "):" << buffer << "\r\n";
							}
									

						}
						else {
							ss << "ME:" << buffer << "\r\n";
							std::cout << "Me #" << copy.fd_array[i] << ":" << buffer << "\r\n";
						}

						std::string str_out = ss.str();
						send(out_sock, str_out.c_str(), str_out.size() + 1, 0);

						
					}

			    }

			}
		}
	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listening_sock, &master_read);
	closesocket(listening_sock);

	// Message to let users know what's happening.
	std::string msg = "SERVER:Server is shutting down. Goodbye\r\n";

	while (master_read.fd_count > 0) {
		// Get the socket number
		SOCKET socket = master_read.fd_array[0];

		//Send the goodbye message
		send(socket, msg.c_str(), msg.size() + 1, 0);

		// Remove it from the master file list and close the socket
		FD_CLR(socket, &master_read);
		closesocket(socket);
	}

	//Cleanup winsock
	WSACleanup();

	system("pause");
	return 0;
}
