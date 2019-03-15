#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>

int main_sd; // socket descriptor corr to the original socket created by the server
int connect_sd=-1; // socket descriptor corr to the socket created on connection with a client

# define MAXCHARS 256
# define MAX_CLIENTS 5

// data structure created for storing data when the database file is loaded.
struct product
{
	int UPC;
	double pc;
	char product_type[MAXCHARS];	
};

struct product db[256];
int total_entries; 


// deciding action to be taken on the basis of request type
void request_type_action(int request_type,int product_code,int number,double *t){
	int pos;
	char message[MAXCHARS];
	switch(request_type){
	    case 0:
		{
			int flag3=0;
			int cnt;
			for(cnt=0;cnt<total_entries;cnt++)
			{
				if(product_code!=db[cnt].UPC)
				{
					continue;
				}
				else{
					pos=cnt;
					flag3=1;
					break;
				}
			}
			
			if(flag3==1)
			{
				*t=*t+(db[pos].pc * number);
				sprintf(message,"0 %f %s\n",db[pos].pc,db[pos].product_type);
				write(connect_sd,message,MAXCHARS);
			}
			else
			{
				sprintf(message,"1 UPC isn't found in database\n");
				write(connect_sd,message,MAXCHARS);
			}
			break;
		}
		case 1:
		{
			sprintf(message,"0 %f\n",*t);
			write(connect_sd,message,MAXCHARS);
			close(connect_sd);
			exit(1);
			break;
		}
		default:
		{
			strcpy(message,"1 Protocol Error\n");
			write(connect_sd,message,MAXCHARS);
		}
	}
}


// servicing of the client request by the corresp. child process created by the server.
void service_request_child_proc(int connect_sd)
{
	const char delimiter[2]=" ";
	char *token;
	char buffer[MAXCHARS],message[MAXCHARS];
	int length,t_arg,request_type;
	int number,product_code;
	double total=0.0;
	while(1)
	{
		memset(message,'\0',MAXCHARS); 
		length=read(connect_sd,buffer,MAXCHARS);
		int flag1=length<0;
		if(flag1)
		{
			strcpy(message,"Error in command from the client\n");
			write(connect_sd,message,MAXCHARS);
			close(connect_sd);
			exit(1);
		}

        int flag2=(strcmp(buffer,"Termination of client")==0);
		if(flag2)
		{
			close(connect_sd);
			printf("\nClient process corresp. to pid %d has been terminated\n",getpid());
			exit(1);
		}

		token=strtok(buffer,delimiter);

        t_arg=0;
        int flag=0;
		while(token!=NULL)
		{
			if(t_arg==0)
				request_type=atoi(token);
			else if(t_arg==1)
				product_code=atoi(token);
			else if(t_arg==2){
				flag=1;
				number=atoi(token);
			}
			else if (t_arg>2){
				flag=0;
				break;
			}
			token=strtok(NULL,delimiter);
			t_arg=t_arg+1;
		}

		if(flag==0)
		{
			strcpy(message,"1 Protocol Error\n");
			write(connect_sd,message,MAXCHARS);
		}
		else
		{   
            request_type_action(request_type,product_code,number,&total);	
		}
	}
}

/*Involves connecting to a client if any and creation of child process for interaction with the client 
after succesful interaction*/

void service_mode() 
{
    pid_t pid;
	while(1)
	{
		int client_len; 
		struct sockaddr_in client_addr;
		client_len=sizeof(client_addr);
		connect_sd=accept(main_sd,(struct sockaddr *)&client_addr,&client_len);
        int f1=(connect_sd==-1);
        if(f1){
        	perror("Unable to establish the connection!");
        	return;
        }

        pid=fork();
        int f2=(pid==0);
		if(f2)
		{    
			printf("\nChild process having pid %d created to service request \n",getpid());
			// Closing the copy of listening_socket held by the child process.
			close(main_sd); 
			service_request_child_proc(connect_sd);                
            // Closing of the connected socket by the child process.
			close(connect_sd); 
			exit(1); 
		}


		/* Inorder to prevent the child process from becoming a zombie, we are issuing non-blocking waitpid() calls*/

		/* 2 waitpid calls are used instead of one since it might be possible that child process hasn't terminated by the 
		 time 1st waitpid is invoked, so, it could be used to kill other child processes if they have terminated*/

		int status;
		pid_t pid_a=waitpid(-1,&status,WNOHANG);
		pid_t pid_b=waitpid(-1,&status,WNOHANG);
		close(connect_sd); 
	}

}

// signal handler to be used incase of Ctlr+C command
void handle_signal(int sig)   
{
	close(main_sd);
	printf("\n---- Server with pid %d terminating ----\n",getpid());
    char message[MAXCHARS]="Server is down\n";
    if(connect_sd!=-1){
		write(connect_sd,message,MAXCHARS);
		close(connect_sd);
    }
	exit(0);
	return;
}

// printing the contents of database array
void print_db()
{

    printf("+-----+--------------+---------+\n");
    printf("| UPC | Product_Name |  Price  |\n");
    printf("+-----+--------------+---------+\n");
	int cnt;
	for(cnt=0;cnt<total_entries;cnt++)
	{
		printf("| %d | %12s | %.2f |\n",db[cnt].UPC,db[cnt].product_type,db[cnt].pc);
	}
    printf("+-----+--------------+---------+\n");
}

//For loading database file into the struct database array
void db_load(char * path){
	FILE *fp=fopen(path,"r");
    if(fp==NULL){
         printf("File doesn't exist\n");
         return;
    }
    //printf("In db\n");
    int count=0;
    while(1){
        int code;
        char it_name[15];
        double price;
        int val=fscanf(fp,"%d,%[^,],%lf\n",&code,it_name,&price);
        //printf("In loop\n");
	    if(val<2){
		    break;
	    }
	    db[count].UPC=code;
	    //printf("%d\n",db[count].UPC);
	    strcpy(db[count].product_type,it_name);
	    db[count].pc=price;
	    count++;
	    //printf("%d\n",count);
    }
    total_entries=count;
    //printf("%d",total_entries);
    fclose(fp);
}

int main(int argc, char *argv[])
{	
	struct sockaddr_in server_addr;
	int server_port;

    if(argc>2){
        printf("\nYou have given more args than required\n");
        printf("Server exiting\n");
	    exit(1);
    }
	else if(argc<2)
	{
		printf("\nYou have given lesser args than required\n");
        printf("Server exiting\n");
		exit(1);
	}
    else{
        server_port = atoi(argv[1]);
    }

    // domain;type;protocol
	// AF_INET -- IPv4 ; SOCK_STREAM -- TCP ; 0 -- Internet Protocol 
	if((main_sd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("Unable to create socket");
		return 0;
	}
    
    server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	//printf("IP Addr %s\n",inet_ntoa(*(struct in_addr *)&server_addr.sin_addr.s_addr));
	server_addr.sin_family=AF_INET;
	
	//bind of the server socket to the given address.
	int bind_res=bind(main_sd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(bind_res==-1){
		
		perror("Unable to bind the socket");
		exit(1);
	}

    /*Putting a cap on the max no of clients with whom the server can connect concurrently and using listen, the server 
    indicates that it's ready for the conn*/
	listen(main_sd,MAX_CLIENTS);

	db_load("database.csv");
    print_db();

    signal(SIGTERM,handle_signal);
	signal(SIGINT,handle_signal);

    service_mode();
    return 0;
}