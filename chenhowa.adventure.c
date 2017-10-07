/* File name: chenhowa.adventure.c
 * Author: Howard Chen
 * Date Created: 7-18-2017
 * Last Modified: 7-23-2017
 * Description: Uses 7 room files located in ./chenhowa.rooms.<PROCESS ID>, to generate
 * 	a dungeon. This program then provides the user with an interface to explore
 * 	and complete that dungeon.
 *
 * 	Note that this program will use the most recent ./chenhowa.rooms.<PROCESS ID> directory
 * 	as the source for its room files
 *
 */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>

#define START_ROOM 1
#define MID_ROOM 2
#define END_ROOM 3
#define NUM_ROOMS 7
#define NUM_NAMES 10
#define MAX_CONNECTIONS 6
#define MIN_CONNECTIONS 3

/*Declares a global mutex -- I'd rather not use this, but in the process of
 * trying to figure out how to make the unlock and lock work (eventually I discovered
 * that I had to make the main thread sleep in my case), I decided using a 
 * global variable to simplify things was the best choice for this assignment.*/
pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

/*struct that contains the information in Room */
struct Room {
	char name[50]; /*name of room */
	char connections[MAX_CONNECTIONS][50]; /*names of rooms that are connected to this room */
	int type; /*type of the room */
	int numConnections; /*Number of rooms connected to this room */

};

/*Player struct that holds player data */
struct Player {
	struct Room* curRoom; /*current room */
	char *history; /*string containing the history of rooms the player has visited */
	int visited; /*total number of rooms the Player has visited */
};


/* Reads in a Room's data from a Room file
 * args: [1] file, a pointer to an opened FILE
 *	[2] room, a pointer to a Room
 * pre: file should point to a Room file created by chenhowa.buildrooms
 * post: data in the Room file has been read into the Room struct
 * ret: none
 */
void readRoom(FILE *file, struct Room *room) {
	char type[50];
	int trash = 0;

	memset(type, '\0', sizeof(type) );

	/*First, scan in the name */
	fscanf(file, "ROOM NAME: %s\n", room->name);

	/*Then repeatedly scan in connections */	
	while(fscanf(file, "CONNECTION %i: %s\n", &trash, room->connections[room->numConnections] ) == 2) {
		room->numConnections++;

	}

	/*Once you're done reading all the connections, read the room type */
	fscanf(file, "ROOM TYPE: %s\n", type);
	if (strcmp( type, "MID_ROOM") == 0 ) {
		room->type = MID_ROOM;
	}
	else if(strcmp( type, "START_ROOM") == 0) {
		room->type = START_ROOM;
	}
	else if(strcmp( type, "END_ROOM") == 0) {
		room->type = END_ROOM;
	}
	else {
		fprintf(stderr, "ERROR in reading room type for %s\n", room->name);
	}

}

/*Initialize a Room struct to good default values, like 0 connections,
 * strings full of null terminators, etc. */
void initRoom(struct Room *room) {
	int i;

	memset(room->name, '\0', 50);
	for( i = 0; i < MAX_CONNECTIONS; i++) {
		memset(room->connections[i], '\0', 50);
	}

	room->type = -1;
	room->numConnections = 0;
}

/*prints the contents of a Room struct to a given open file */ 
void printRoom(FILE *file, struct Room *r) {
	int i;
	char* type_str;

	/*print the Room's name, or NULL if the room has no name */
	if( r->name[0] == '\0') {
		fprintf(file, "NULL\n");
	} else {
		fprintf(file, "ROOM NAME: %s\n", r->name);
	}

	/*Print each of the room's existing connections, or print NULL CONNECTION
 * 	if somehow a room connection is null */
	for (i = 0; i < r->numConnections; i++) {
		if(r->connections[i][0] == '\0') {
			fprintf(file, "NULL CONNECTION\n");
		} else {
			fprintf(file, "CONNECTION %i: %s\n", i + 1, r->connections[i]);
		}
	}
	
	/*Print the room's type */
	switch (r->type) {
		case 1: type_str = "START_ROOM";
		break;
		case 2: type_str = "MID_ROOM";
		break;
		case 3: type_str = "END_ROOM";
		break;
		default: type_str = "UNASSIGNED_ROOM";
		break;
	}

	/*flush the buffer to write to the file */
	fprintf(file, "ROOM TYPE: %s\n", type_str);
	fflush(file);
}


/*Prompts the user for a command using the data contained in the
 * player's current room member variable */
void promptPlayer(struct Player* player) {
	int i;
	printf("CURRENT LOCATION: %s\n", player->curRoom->name);
	printf("POSSIBLE CONNECTIONS:");
	for(i = 0; i < player->curRoom->numConnections; i++) {
		printf(" %s", player->curRoom->connections[i]);
		if(i < player->curRoom->numConnections - 1) {
			printf(",");
		} else {
			printf(".");
		}
	}
	printf("\n");
	printf("WHERE TO? >");

}

/* gets a single line of input from an opened input file
 * args: [1] file, a pointer to an open file
 * pre: file is an open file, where every line ends in a newline
 * post: a single line will be read from the file
 * post: returned c-string MUST be freed
 * post: newline is not included in the returned string
 * ret: a pointer to the line that was read, which was allocated on the heap.
 *
 * NOTE: this method comes from the lecture slides of CS 344 that describe
 * 	how to use getline to get a string of input from an open File.
 */
char* getInput(FILE* file) {
	int numChars = -5;
	size_t bufferSize = 0;
	char* line = NULL;

	/*Get a line of user input ending in '\n' */
	numChars = getline(&line, &bufferSize, file);
	
	/*Remove the newline from the end of the string */
	line[numChars - 1] = '\0';
	
	return line;
}

/* Executes the command input by the user
 * args: [1] player, the struct containing the player's data
 * 	[2] string, a cstring pointing to the user's command
 * 	[3] rooms, an array of room structs that the user is trying to navigate through
 * pre: rooms and player must have been completely initialized
 * post: user's current room is updated if user input was valid, or the time is
 * 	shown to the user. Otherwise an error message is given to the user.
 * ret: an int indicating whether or not an input error occured -- 1 represents error
 * 	0 represents successful user input
 */
int executeCommand(struct Player* player, char* string, struct Room* rooms) {
	int i;
	int j;
	char* time;
	FILE* file;

	/*If player chose to view time, we open the file, unlock the mutex so the thread
 * 	can write to it, sleep to give the other thread time to acquire the lock, then
 * 	relock the mutex, close the file, open for reading, read in the value
 * 	and then close the file again */
	if(strcmp(string, "time") == 0) {
		/*Sleep two seconds to let the other thread obtain the lock and write the time */
		pthread_mutex_unlock(&fileMutex);
		sleep(2);
		pthread_mutex_lock(&fileMutex);

		/*After regaining the lock, read the current time from file */
		file = fopen("currentTime.txt", "r");
		time = getInput( file );
		fprintf(stdout, "\n%s\n\n", time);

		free(time);
		time = NULL;
		fclose(file);
		return 0;


	}
	
	for(i = 0; i < player->curRoom->numConnections; i++) {
		/*If the input string was one of the possible connections, change
 * 			the player's current room to point to the room with that name */
		if(strcmp(player->curRoom->connections[i], string) == 0) {
			for(j = 0; j < NUM_ROOMS; j++) {
				if(strcmp(rooms[j].name, string) == 0) {
					player->curRoom = rooms + j;
					player->visited++;

					/*Update the player's room history by reallocating
 * 						the history string */
					player->history = realloc(player->history, 25 * player->visited);
					strcat(player->history, string);
					strcat(player->history, "\n");
				}

			}
			printf("\n");
			return 0;
		}
	}	

	/*Otherwise, the string was not a command or a room name, so print
 * 		and error message and return 1 */
	printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN\n\n");

	return 1;
}

/*Method of getting time is from
 * https://stackoverflow.com/questions/1442116/how-to-get-date-and-time-value-in-c-program
 *
 * Description: this function writes the current LOCAL time in the assignment format to a file called
 * 	"currentTime.txt"
 * Args: none
 * Pre: requires a global mutex called fileMutex. This function must acquire a lock on this mutex
 * 	before writing to the file
 * Post: this function executes indefinitely, repeatedly opening "currentTime.txt" and writing
 * 	the time, IF it can get the lock again.
 * ret: none 
 * */
void *writeTime(void *args) {
	time_t t;
	struct tm local_time;
	FILE *file;
	while(1) {
		
		/*Attempt to gain a lock on writing to the file */
		pthread_mutex_lock( &fileMutex );

		/* Once the lock is gained, open the deisred file */
		t = time(NULL);
		local_time = *localtime(&t);
		file = fopen("currentTime.txt", "w");

		/*Convert hour values to correct format while writing to file */
		if(local_time.tm_hour == 0) {
			fprintf(file, "12");
		} else if (local_time.tm_hour <= 12) {
			fprintf(file, "%d", local_time.tm_hour);
		} else {
			fprintf(file, "%d", local_time.tm_hour - 12);
		}
		fprintf(file, ":");

		/*Convert minute values to correct format while writing to file*/
		fprintf(file, "%02d", local_time.tm_min);
		
		/*Print am/pm status*/
		if(local_time.tm_hour >= 12) {
			fprintf(file, "pm");
		}
		else {
			fprintf(file, "am");
		}

		fprintf(file, ", ");

		/*Write the current day of the week */
		switch(local_time.tm_wday) {
			case 0: fprintf(file, "Sunday");
			break;
			case 1: fprintf(file, "Monday");
			break;
			case 2: fprintf(file, "Tuesday");
			break;
			case 3: fprintf(file, "Wednesday");
			break;
			case 4: fprintf(file, "Thurday");
			break;
			case 5: fprintf(file, "Friday");
			break;
			case 6: fprintf(file, "Saturday");
			break;
			default: fprintf(stderr, "ERROR IN WEEKDAY\n\n");
		}
		fprintf(file, ", ");
		
		/*Write the month */
		switch(local_time.tm_mon) {
			case 0: fprintf(file, "January");
			break;
			case 1: fprintf(file, "February");
			break;
			case 2: fprintf(file, "March");
			break;
			case 3: fprintf(file, "April");
			break;
			case 4: fprintf(file, "May");
			break;
			case 5: fprintf(file, "June");
			break;
			case 6: fprintf(file, "July");
			break;
			case 7: fprintf(file, "August");
			break;
			case 8: fprintf(file, "September");
			break;
			case 9: fprintf(file, "October");
			break;
			case 10: fprintf(file, "November");
			break;
			case 11: fprintf(file, "December");
			break;
			default: fprintf(stderr, "ERROR IN MONTH\n\n");
		}
		fprintf(file, " ");

		/*Write the day of the month, and the year */
		fprintf(file, "%d", local_time.tm_mday);
		fprintf(file, ", %d\n", local_time.tm_year + 1900);

		/*Once the writing is done, close the file, unlock the file for reading,
 * 		and SLEEP so the main thread can regain the lock */
		fclose(file);
		pthread_mutex_unlock( &fileMutex );
		sleep(2);
	}
}


int main() {
	/*the directory manipulation code used here is DIRECTLY based on the code
 * 		provided in the Reading Notes for CS 344 on Canvas */

	int newestDirTime = -1;
	char targetDirPrefix[64] = "chenhowa.rooms."; /*Desired prefix of directory*/
	char newestDirName[256]; /*holds name of newest dir so we can open it later */

	DIR* dirToCheck; /*Holds my starting directory */
	struct dirent *fileInDir; /*Hold current file in starting directory */
	struct stat dirAttributes; /*Holds info from stat call on fileInDir */

	FILE *fd; /*File descriptor for opening and closing room files */
	char fileName[256]; /*Holds a given room file's name. */

	struct Room rooms[NUM_ROOMS]; /*Hold the rooms data in this program */
	int i;		/*index for the rooms array */

	struct Player player; /*Holds the player data */
	char* playerInput;  /*Holds the user input during the game's execution */

	int threadResult; /*holds result of creating thread */
	pthread_t timeThread; /*thread! */

	/*Null terminate string memory to avoid problems later */
	memset(fileName, '\0', sizeof(fileName));
	memset(newestDirName, '\0', sizeof(newestDirName));

	/*initialize all the rooms */
	for(i = 0; i < NUM_ROOMS; i++) {
		initRoom(rooms + i);
	}



	dirToCheck = opendir(".");  /*Open the current working directory as the starting dir */
	if(!dirToCheck) {
		fprintf(stderr, "Error. Couldn't open current working directory");
		return 1;
	}

	/*check every entry in the directory to see if it contains the prefix*/
	fileInDir = readdir(dirToCheck);
	while(fileInDir != NULL) { 
		/*If the entry has the prefix in its name, get the attributes of this entry */
		if(strstr(fileInDir->d_name, targetDirPrefix) != NULL) { 
			stat(fileInDir->d_name, &dirAttributes);

			if( (int)dirAttributes.st_mtime > newestDirTime ) {
				/*if this entry was created later, then it is the newest directory
 * 					with the desired prefix*/
				newestDirTime = (int)dirAttributes.st_mtime;
				memset(newestDirName, '\0', sizeof(newestDirName));
				strcpy(newestDirName, fileInDir->d_name);

			}
		}
		
		fileInDir = readdir(dirToCheck);
	}

	/*After examining all the MATCHING files in the current directory, close it */
	closedir(dirToCheck);
	dirToCheck = NULL;

	/*Now that we have the newest directory, we can open it and scan the contents */
	dirToCheck = opendir(newestDirName);
	if(!dirToCheck) {
		fprintf(stderr, "Error. Couldn't open NEWEST directory");
		return 1;
	}

	i = 0; /*Prepare to read in room files to the rooms array */

	/*Read every Room file in the directory. Use each room file to set the values of a new
 * 		Room struct in the Room array */
	fileInDir = readdir(dirToCheck);
	while(fileInDir != NULL) { 
		memset(fileName, '\0', sizeof(fileName));
		/*We care about every file except the . and .. files */
		if(strcmp(fileInDir->d_name, ".") != 0 && strcmp(fileInDir->d_name, "..") != 0  ) {
			sprintf(fileName, "./%s/%s", newestDirName, fileInDir->d_name);

			/*Open the room file, and if the open is successful, read in the Room data */
			fd = fopen(fileName, "r"); 
			
			if(!fd) {
				fprintf(stderr, "Error. Attempt to open room file %s failed", fileName);
				return 1;
			}	

			readRoom(fd, rooms + i);
			i++;
			
			/*Close the room file once we're done with it */
			fclose(fd);
		}
		/*Get the next room file */
		fileInDir = readdir(dirToCheck);
	}
	/*Close the directory when done. Now ready to begin the game */
	closedir(dirToCheck);
	dirToCheck = NULL;

	/*To begin the game, initiate the player to the required values:
 * 		give the player an empty history, with 0 rooms visited
 * 		and the correct starting room */
	player.history = malloc(25 * sizeof(char) );
	memset(player.history, '\0', 25);
	player.visited = 0;
	for(i = 0; i < NUM_ROOMS; i++) {
		if( rooms[i].type == START_ROOM ) {
			player.curRoom = rooms + i;
		}
	}

	/*Before starting the thread, LOCK THE FILE */
	pthread_mutex_lock( &fileMutex );

	/*Create a thread to constantly try to write and read a current time to a file */
	threadResult = pthread_create( &timeThread, NULL, writeTime, NULL);

	/*While the current room of the player is NOT the end room, play the game */
	while(player.curRoom->type != END_ROOM) {
		/* get player input */
		promptPlayer(&player);
		playerInput = getInput(stdin);	

		/*Execute the command specified by the player input */
		executeCommand(&player, playerInput, rooms);

		/*Prepare to get new user input */
		free(playerInput);
		playerInput = NULL;
	}

	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %i STEPS. YOUR PATH TO VICTORY WAS:\n", player.visited);
	printf("%s", player.history);

	/*Clean up the allocated memory of the player's history cstring */
	free(player.history);

	/*End the other thread */
	pthread_cancel(timeThread);
	sleep(8);

	return 0;
}

