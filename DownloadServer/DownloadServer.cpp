#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable : 4996)

int computeChecksum(char* fname, long fsize, UINT32* checksum)
{
	FILE *fp;
	int ret = -1, fread_byte, i = 0;
	unsigned char buf[512];
	fp = fopen(fname, "rb");
	if (!fp || ! checksum)
		goto exit;
	else{
		while ((fread_byte = fread(buf, sizeof(char), 512, fp)) > 0){
			for (i = 0; i < fread_byte; i++){
				*checksum += buf[i];
			}
		}		
		printf("%s():checksum 0x%x \n", __FUNCTIONW__, *checksum);
		ret = 0;
	}

exit:
	return ret;
}

int main(int argc, char *argv[])
{
	WSADATA wsa;
	SOCKET server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	int port, addr_len;
	FILE *fp;
	long fsize;
	char *fname = NULL, buf[1024];
	size_t fread_bytes;
	UINT32 checksum = 0;

	if (argc == 3) {
		port = atoi(argv[1]);
		fname = argv[2];
	}
	else {
		printf("Usage: %s PORT FILE\n", argv[0]);
		return 1;
	}

	if ((fp = fopen(fname, "rb")) == NULL) {
		printf("Fail to open file %s\n", fname);
		return 1;
	}
	else {
		fseek(fp, 0L, SEEK_END);
		fsize = ftell(fp);
		fclose(fp);
	}
	if (computeChecksum(fname, fsize, &checksum)){
		printf("Fail to compute checksum\n");
		return 1;
	}


	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Fail to initialize Winsock\n");
		return 1;
	}

	if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)	{
		printf("Can not create socket\n");
		return 1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	memset(server_addr.sin_zero, 0x00, 8);

	if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		printf("Fail to bind socket address\n");
		closesocket(server_socket);
		return 1;
	}

	listen(server_socket, 10);
	printf("Listening on port (%d) to send %s (%d bytes)\n\n", port, fname, fsize);

	addr_len = sizeof(struct sockaddr_in);	
	while (1) {
		printf("Waiting for client ...\n");
		client_socket = accept(server_socket, (struct sockaddr *)& client_addr, &addr_len);

		if (client_socket == INVALID_SOCKET) {
			printf("Fail to accept client connection\n");
		}
		else {
			printf("Accept client connection from %s\n", inet_ntoa(client_addr.sin_addr));

			if (checksum){
				int send_checksum = 0;
				printf("Send checksum and file size first\n");
				// ¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X
				//¡W checksum ¡Wpadding 0 ¡Wfile size ¡W
				// ¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X¡X
				memcpy(buf, &checksum, sizeof(UINT32));
				memset(buf + sizeof(UINT32), 0, sizeof(UINT32));
				memcpy(buf + sizeof(UINT32) * 2, &fsize, sizeof(long));
				send_checksum = send(client_socket, buf, sizeof(UINT32) * 2 + sizeof(long), 0);
				printf("Send checksum byte %d\n", send_checksum);
			}

			if ((fp = fopen(fname, "rb")) == NULL) {
				printf("Fail to open file %s\n", fname);
			}
			else {
				int send_bytes = 0;
				printf("Sending file... \n");

				while ((fread_bytes = fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
					send_bytes += send(client_socket, buf, fread_bytes, 0);
					printf(".");
				}

				fclose(fp);
				printf("\nTotal send %u bytes\n", send_bytes);
			}

			while (true) {
				int bytes = recv(client_socket, buf, sizeof(buf), 0);
				if (bytes > 0) {
					// got data from the client.
				}
				else if (bytes == 0) {
					// client disconnected.
					printf("Client Disconnected.\n");
					break;
				}
			}
			closesocket(client_socket);
		}
	}

	return 0;
}
