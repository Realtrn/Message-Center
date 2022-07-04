#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <signal.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <termios.h>

#define LENGTH 2048



// Global variables

volatile sig_atomic_t flag = 0;

int sockfd = 0;

char name[32];

volatile int in_task = 0 ; 

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct termios old, new;



void initTermios(int echo)

{

    tcgetattr(0, &old); //grab old terminal i/o settings

    new = old; //make new settings same as old settings

    new.c_lflag &= ~ICANON; //disable buffered i/o

    new.c_lflag &= echo ? ECHO : ~ECHO; //set echo mode

    tcsetattr(0, TCSANOW, &new); //apply terminal io settings

}



/* Restore old terminal i/o settings */

void resetTermios(void)

{

    tcsetattr(0, TCSANOW, &old);

}



/* Read 1 character - echo defines echo mode */

char getch_(int echo)

{

    char ch;

    initTermios(echo);

    ch = getchar();

    resetTermios();

    return ch;

}



/* 

Read 1 character without echo 

getch() function definition.

*/

char getch(void)

{

    return getch_(0);

}



/* 

Read 1 character with echo 

getche() function definition.

*/

char getche(void)

{

    return getch_(1);

}



void str_overwrite_stdout()

{

	printf("%s", "> ");

  	fflush(stdout);

}



void str_trim_lf (char* arr, int length)

{

	int i;

	for (i = 0; i < length; i++) {

    		if (arr[i] == '\n') {

      			arr[i] = '\0';

      			break;

    		}

  	}

}



void catch_ctrl_c_and_exit(int sig) 

{

	flag = 1;

}



void send_msg_handler()

{

	char message[LENGTH] = {};

	char buffer[LENGTH + 32] = {};

	 

	 

	while(1) {	

		str_overwrite_stdout();

			int k = 0 ;

			char c = 0 ;

			while(c != '\n')

			{

				if(k > 0) in_task = 1 ; 

			

				

				c = getche() ; 

				message[k] = c ;

				k ++ ;  

			}

			in_task = 0 ; 

			

    		str_trim_lf(message, LENGTH);



    		if (strcmp(message, "exit") == 0) {

			break;

    		} else {



      			sprintf(buffer, "%s: %s\n", name, message);

      			send(sockfd, buffer, strlen(buffer), 0);

				// in_task = 0 ; 

    		}



		bzero(message, LENGTH);

    		bzero(buffer, LENGTH + 32);

		usleep(1000);

  	}

	

  	catch_ctrl_c_and_exit(2);

}



void recv_msg_handler()

{

	char message[LENGTH] = {};

  	while (1) {

		int receive = recv(sockfd, message, LENGTH, 0);

    		if (receive > 0) {

				 while(in_task == 1) ; 

      			printf("%s", message);

      			str_overwrite_stdout();

    		} else if (receive == 0) {

			break;

    		} else {

			// -1

		}

		memset(message, 0, sizeof(message));

  	}

}



int main(int argc, char **argv)

{

	if(argc != 3) {

		printf("Usage: %s <ip> <port>\n", argv[0]);

		return EXIT_FAILURE;

	}



	char *ip = argv[1];

	int port = atoi(argv[2]);



	signal(SIGINT, catch_ctrl_c_and_exit);



	printf("Please enter your name: ");

  	fgets(name, 32, stdin);

  	str_trim_lf(name, strlen(name));



	if (strlen(name) > 32 || strlen(name) < 2) {

		printf("Name must be less than 32 and more than 2 characters.\n");

		return EXIT_FAILURE;

	}



	printf("Select your role:\n");

	printf("1. I am Talker!\n");

	printf("2. I am Listener!\n");

	printf("3. I am both Talker and Listener\n");

	char role;

	printf("Press your choosen: ");

	scanf("%c", &role);

	while (role != '1' && role != '3' && role != '2') {

		printf("Your selection is invalid!\n");

		printf("Press your choosen again: ");

		scanf("%c", &role);

	}

	struct sockaddr_in server_addr;

	

	/* Socket settings */

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

  	server_addr.sin_family = AF_INET;

	server_addr.sin_addr.s_addr = inet_addr(ip);

	server_addr.sin_port = htons(port);



  	// Connect to Server

  	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  	if (err == -1) {

		printf("ERROR: connect\n");

		return EXIT_FAILURE;

	}



	// Send name

	char name_cp[32] ; 

	for(int i = 0 ; i < 32 ; i ++)

	{

		if(i == 0) name_cp[i] = role ; 

		else

		{

			name_cp[i] = name[i - 1] ; 

		}

	}

	send(sockfd, name_cp, 32, 0);



	printf("===== WELCOME TO THE CHATROOM =====\n");

	



	pthread_t send_msg_thread;

	pthread_t recv_msg_thread;



	switch(role) {

	case '1':

		printf("Your role is Talker\n");

		printf("===================================\n");

  		if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0) {

			printf("ERROR: pthread\n");

    			return EXIT_FAILURE;

		}

		break;	

	case '2':

		printf("Your role is Listener\n");

		printf("===================================\n");

  		if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {

			printf("ERROR: pthread\n");

			return EXIT_FAILURE;

		}

		break;

	case '3':

		printf("Your role is both Talker and Listener\n");

		printf("===================================\n");

  		if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0) {

			printf("ERROR: pthread\n");

    			return EXIT_FAILURE;

		}



  		if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {

			printf("ERROR: pthread\n");

			return EXIT_FAILURE;

		}

		break;

	default:

		break;

	}



	while (1) {

		if(flag) {

			printf("\nBye\n");

			break;

    		}

	}



	close(sockfd);



	return EXIT_SUCCESS;

}

