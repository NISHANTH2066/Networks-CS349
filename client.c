#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>

int main_sd; 

#define MAXCHARS 256

// signal handler to be used incase of Ctlr+C command
void handle_signal(int sig)
{
	printf("\nClient terminating\n");
    char message[MAXCHARS]="Termination of client";
	write(main_sd,message,MAXCHARS);

	close(main_sd);
	exit(0);
	return;
}


// Connection of client to the server and accordingly,issuing the requests

void client_service_mode()
{
	
	char receive_buffer[MAXCHARS];
	char input_buffer[MAXCHARS];

	printf("Request in the following format:\n");
	printf("+----------------+-------------+----------+\n");
	printf("| <Request_Type> | <UPC-Code>  | <Number> |\n");
	printf("+----------------+-------------+----------+\n\n");

	while(1)
	{   
		memset(receive_buffer,'\0',MAXCHARS);
		memset(input_buffer,'\0',MAXCHARS);
		//fgets(input_buffer,MAXCHARS,stdin);
		char buff_char;
		scanf("%[^\n]%c",input_buffer,&buff_char);
		
		write(main_sd,input_buffer,MAXCHARS);
		read(main_sd,receive_buffer,MAXCHARS);
        
        // deciding action to be taken on the basis of response type
        char response_type=receive_buffer[0];
        switch(response_type){
		    case '0':{
				//fputs(receive_buffer,stdout);
				printf("%s\n",receive_buffer);
				const char delimiter[2]=" ";
				char * token;
				token=strtok(receive_buffer,delimiter);
				int token_count=0;
				while(token!=NULL)
				{	
				   token=strtok(NULL,delimiter);
				   token_count=token_count+1;
			    }
			    if(token_count==2){
	                printf("--- Client exiting ---\n\n");
			        close(main_sd);
			        exit(0);
			        return;
			    }
			    break;
			}
			case '1':
			{
				//fputs(receive_buffer,stdout);	
				printf("%s\n",receive_buffer);
				break;
			}
			default:
			{	
				//fputs(receive_buffer,stdout);
				printf("%s\n",receive_buffer);
				printf("Client exiting\n");
				close(main_sd);
				exit(0);
				return;
			}
		}
	}	
}



int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
	int server_port;
    if(argc>3){
        printf("\nYou have given more args than required\n");
        printf("--- Client exiting ---\n\n");
	    exit(0);
    }
	else if(argc<3)
	{
		printf("\nYou have given lesser args than required\n");
        printf("--- Client exiting ---\n\n");
		exit(0);
	}
    else{
        server_port = atoi(argv[2]);
    } 
	if((main_sd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("Unable to create socket");
		return 0;
	}
    
    server_addr.sin_port = htons(server_port); 
	server_addr.sin_addr.s_addr = inet_addr(argv[1]); 
	server_addr.sin_family=AF_INET;

	int connect_res=connect(main_sd,(struct sockaddr *) &server_addr,sizeof(server_addr));
	if(connect_res<0){
		perror("Unable to connect to the server");
	    return 0;
	}

	//terminate this process on Ctrl+c
    signal(SIGTERM,handle_signal);
	signal(SIGINT,handle_signal);

	printf("\n--- Connected to the server ---\n\n");
	client_service_mode();
	return 0;
}