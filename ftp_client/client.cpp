#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define bufferSize 1024

void sendCommand(int sockfd, char* command)
{
    char buffer[100] = {0};
    strcpy(buffer, command);
	int len = strlen(buffer);
    if (send(sockfd, buffer, len, 0) < len)
        printf("Send command error.\n");
}

void sendFile(int sockfd, char* filename)
{
	char buffer[bufferSize] = {0};
	recv(sockfd, buffer, bufferSize, 0);
    printf("%s\n", buffer);
	FILE* fp;
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
        printf("Can not open file: %s.\n", filename);
		long fileLen = 0L;
		char sendLen[100] = {0}; 
		sprintf(sendLen, "%ld", fileLen);
		send(sockfd, sendLen, strlen(sendLen), 0);
        return;
    }
	else
	{
		fseek(fp, 0L, SEEK_END);
		long fileLen = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
		char sendLen[100] = {0}; 
		sprintf(sendLen, "%ld", fileLen);
		send(sockfd, sendLen, strlen(sendLen), 0);
        //printf("fileLen:%ld\n", fileLen);
		memset(buffer, 0, bufferSize);
		recv(sockfd, buffer, bufferSize, 0);
		memset(buffer, 0, bufferSize);
		int len;
		while ((len = fread(buffer, 1, bufferSize, fp)) > 0)
		{
		    send(sockfd, buffer, len, 0);
		    memset(buffer, 0, bufferSize);
			fileLen -= len;
			if (fileLen <= 0)
				break;
		}
	}
    fclose(fp);
}

void recvFile(int sockfd, char* filename)
{
    char buffer[bufferSize] = {0};
	recv(sockfd, buffer, bufferSize, 0);
	long fileLen = atol(buffer);
	if (fileLen == 0L)
	{
		printf("Can not open file: %s.\n", filename);
		return;
	}
	memset(buffer, 0, bufferSize);
	strcpy(buffer, "OK");
	send(sockfd, buffer, bufferSize, 0);
    int outputFile;
    if ((outputFile = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
    {
        printf("Can not open file: %s\n", filename);
        return;
    }
	memset(buffer, 0, bufferSize);
    int len;
    while ((len = recv(sockfd, buffer, bufferSize, 0)) > 0)
    {
        write(outputFile, buffer, len);
        memset(buffer, 0, bufferSize);
		fileLen -= len;
		if (fileLen <= 0)
			break;
    }
    close(outputFile);
}

void cmdHelp(int sockfd)
{
    char buffer[bufferSize] = {0};
    recv(sockfd, buffer, bufferSize, 0);
    printf("%s\n", buffer);
}

void cmdGet(int sockfd, char* filename)
{
	int len = strlen(filename);
	if (send(sockfd, filename, len, 0) < len)
    	printf("Send filename error.\n");
    recvFile(sockfd, filename);
}

void cmdPut(int sockfd, char* filename)
{
	int len = strlen(filename);
	if (send(sockfd, filename, len, 0) < len)
    	printf("Send filename error.\n");
    sendFile(sockfd, filename);
}

void cmdPwd(int sockfd)
{
    char buffer[bufferSize] = {0};
    int len;
    if ((len = recv(sockfd, buffer, bufferSize, 0)) > 0)
    {
        printf("%s\n", buffer);
    }
    else
    {
        printf("Receive error.\n");
    }
}

void cmdDir(int sockfd)
{
    char buffer[bufferSize] = {0};
    recv(sockfd, buffer, bufferSize, 0);
    long fileLen = atol(buffer);
    memset(buffer, 0, bufferSize);
    strcpy(buffer, "OK");
    send(sockfd, buffer, bufferSize, 0);
    memset(buffer, 0, bufferSize);
    int len;
    while ((len = recv(sockfd, buffer, bufferSize, 0)) > 0)
    {
        printf("%s", buffer);
        memset(buffer, 0, bufferSize);
        fileLen -= len;
        if (fileLen <= 0)
            break;
    }
}

void cmdCd(int sockfd, char* dir)
{
	int len = strlen(dir);
	if (send(sockfd, dir, len, 0) < len)
    	printf("Send filename error.\n");
    char buffer[bufferSize] = {0};
    if ((len = recv(sockfd, buffer, bufferSize, 0)) > 0)
    {
        printf("%s\n", buffer);
    }
    else
    {
        printf("Receive error.\n");
    }
}

int main(int argc, char** argv)
{
    //usage
    if (argc < 3)
    {
        printf("Usage: client <address> <port>\n");
        return 0;
    }
    //create client socket
    int clientMsg, clientData;
    clientMsg = socket(AF_INET, SOCK_STREAM, 0);
    clientData = socket(AF_INET, SOCK_STREAM, 0);
    if (clientMsg < 0 || clientData < 0)
    {
        fprintf(stderr, "Fail to initial socket.\n");
        exit(2);
    }
    //connect to server
    sockaddr_in serverMsg, serverData;
    
    serverMsg.sin_family = AF_INET;
    serverMsg.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverMsg.sin_addr);
    if(connect(clientMsg, (sockaddr*)&serverMsg, sizeof(serverMsg)) < 0)
    {
        perror("Can not connect server.");
        exit(3);
    }
    //parse command
    char command[10] = {0};
    char args[50] = {0};
	strcpy(command, "PASV");
	send(clientMsg, command, strlen(command), 0);
	recv(clientMsg, args, 50, 0);
	int port = atoi(args);
	serverData.sin_family = AF_INET;
    serverData.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &serverData.sin_addr);
	if(connect(clientData, (sockaddr*)&serverData, sizeof(serverData)) < 0)
    {
        perror("Can not connect server.");
        exit(3);
    }

    while(1)
    {
		memset(command, 0, sizeof(command));
		memset(args, 0, sizeof(args));
        scanf("%s", command);
        if (!strcmp(command, "get"))
        {
            scanf("%s", args);
            sendCommand(clientMsg, command);
            cmdGet(clientData, args);
        }
        else if (!strcmp(command, "put"))
        {
            scanf("%s", args);
            sendCommand(clientMsg, command);
            cmdPut(clientData, args);
        }
        else if (!strcmp(command, "pwd"))
        {
            sendCommand(clientMsg, command);
            cmdPwd(clientData);
        }
        else if (!strcmp(command, "dir"))
        {
            sendCommand(clientMsg, command);
            cmdDir(clientData);
        }
        else if (!strcmp(command, "cd"))
        {
			scanf("%s", args);
            sendCommand(clientMsg, command);
            cmdCd(clientData, args);
        }
        else if (!strcmp(command, "?"))
        {
			sendCommand(clientMsg, command);
            cmdHelp(clientData);
        }
        else if (!strcmp(command, "quit"))
        {
            sendCommand(clientMsg, command);
            break;
        }
		else
		{
			printf("Invalid command.\n");
		}
    }
    close(clientMsg);
    close(clientData);
}
