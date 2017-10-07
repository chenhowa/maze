/* Filename: chenhowa.buildrooms.c
 * Author: Howard Chen
 * Date Created: 7-11-2017
 * Description: File creates room files for the Program 2 adventure game
 * Input: N/A
 * Output: 7 room files with randomly generated room names and room connections
 *
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


#define START_ROOM 1
#define MID_ROOM 2
#define END_ROOM 3
#define NUM_ROOMS 7
#define NUM_NAMES 10
#define MAX_CONNECTIONS 6
#define MIN_CONNECTIONS 3


/*Room struct to hold room data*/
struct Room {
	char* name;
	struct Room* connections[MAX_CONNECTIONS]; /*Pointer to other, connected rooms */
	int type; /*integer representing the room type */
	int numConnections; /*Number of room's connections */
};

/*Struct for organizing the names and whether or not they've been assigned */
struct OneToOneNameMap {
	char** names; /*Holds an array of char* representing names */
	int* used_names; /*array mapping each name to whether its been used */
	int size; /*total number of names */
};

/*Prints the data in a Room struct to a given opened FILE */
void printRoom(FILE *file, struct Room *r) {
	int i;
	char* type_str;

	/*Print the name of the Room, or NULL if there is no name */
	if( r->name == NULL) {
		fprintf(file, "NULL\n");
	} else {
		fprintf(file, "ROOM NAME: %s\n", r->name);
	}

	/*Print the name of the connected room, or NULL CONNECTION
 * 		if the connected room is somehow NULL */
	for (i = 0; i < r->numConnections; i++) {
		if(r->connections[i] == NULL) {
			fprintf(file, "NULL CONNECTION\n");
		} else {
			fprintf(file, "CONNECTION %i: %s\n", i + 1, r->connections[i]->name);
		}
	}
	
	/*If a room type has been assigned, print it to console */
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
	fprintf(file, "ROOM TYPE: %s\n", type_str);

	/*flush the output to the file */
	fflush(file);
}

/*Simple function for printing out a OneToOneNameMap to ensure that
 * unique names were assigned randomly */
void printNameMap(struct OneToOneNameMap* map) {
	int i;
	for(i = 0; i < map->size; i++) {
		printf("Name %i: %s; Used: %i\n", i + 1, map->names[i], map->used_names[i]);

	}

}

/* Assigns random names from a OneToOneNameMap to an array of rooms
 * Arguments: [1} rooms, an array of Room structs
 * 		[2] count, the number of Rooms
 * 		[3] name_map, holds the names to assign
 * Pre: There should be at least as many names as their are rooms
 * pre: Both the rooms and the map should be initialized with their respective init functions
 * post: random names will have been assigned the rooms. Each Room struct will have a pointer
 * 	to its name
 * ret: none
 *
 */
void assignRandomNames(struct Room* rooms, int count, struct OneToOneNameMap* name_map) {
	/*For every room in rooms, generate a random index that referes to a name*/
	int i;
	for(i = 0; i < count; i++) {
		int unassigned = 1;
		while(unassigned == 1) {
			/*Get a random name */
			int name_index = rand() % name_map->size;
			/*If that name is unused, point the current room to it, and continue
 * 			to the next room. Otherwise, try again for the same room*/
			if(name_map->used_names[name_index] == 0) {
				name_map->used_names[name_index] = 1;
				rooms[i].name = name_map->names[name_index];
				unassigned = 0;
			}
		}
	}	
}

/* Assigns random room types to an array of rooms
 * Arguments: [1] rooms, an array of Room structs
 * 		[2] count, the number of Rooms
 * pre: rooms should be initialized with their init function
 * post: rooms will have been assigned random room types. Exactly 1 room will  be a 
 * 	START_ROOM. Exactly 1 room will be an END_ROOM
 * ret: none
 *
 */
void assignRandomTypes(struct Room* rooms, int count) {
	int i;
	int startRoom = -1;
	int endRoom = -1;

	/*Every room execpt the start room and end room must be MID_ROOMs.
		So assign the midrooms first, and 
		then randomly generate two different indices to be
		START_ROOM and END_ROOM */
	for(i = 0; i < count; i++) {
		rooms[i].type = 2;
	}

	startRoom = rand() % count;
	do {
		endRoom = rand() % count;

	} while(endRoom == startRoom);

	rooms[startRoom].type = 1;
	rooms[endRoom].type = 3;
}

/* Determines whether or not a graph is full 
 * Args: [1] rooms, an array of Room structs
 *	[2] count, the number of rooms
 * pre: rooms should be initialized properly
 * post: may panic if a room has more than MAX_CONNECTION connections
 * ret: 0 if any room has fewer than MIN_CONNECTIONS
 * 	1 if every room has at least MIN_CONNECTIONS, but no more than MAX_CONNECTIONS
 */     
int graphIsFull(struct Room* rooms, int count) {
	int i;
	for(i = 0; i < count; i++) {
		assert(rooms[i].numConnections <= MAX_CONNECTIONS);
		if(rooms[i].numConnections < MIN_CONNECTIONS) {
			return 0;
		}	
	}
	return 1;

}


/*Returns a pointer to a random existing room*/
struct Room* getRandomRoom(struct Room* rooms, int count) {
	int room_index = rand() % count;

	return rooms + room_index;
}

/*Returns true if a room has fewer than MAX_CONNECTIONS
 * Returns false otherwise */
int canAddConnectionFrom(struct Room* room) {
	return room->numConnections < MAX_CONNECTIONS;
}

/*Creates a one-way connection between two rooms, and updates the
 * numConnections of the source room*/
/*WARNING: Does not check if this connection is legal
 * Use canAddConnectionFrom to check beforehand*/
void connectRoom(struct Room* x, struct Room* y) {
	x->connections[x->numConnections] = y;	
	x->numConnections++;
}

/*Returns 1 if two room pointers are pointing at the same room
 * Otherwise returns 0 */
int isSameRoom(struct Room* x, struct Room* y) {
	return x == y;
}

/*Returns 1 if x has no one-way connection to y
 * Otherwise returns 0*/
int unconnected(struct Room* x, struct Room* y ) {
	int i;

	/*Scan all of x's connectiosn and compare them to y */
	for(i = 0; i < x->numConnections; i++) {
		if(x->connections[i] == y) {
			return 0;
		}
	}

	return 1;

}

/* Adds a random connection between two Rooms in an array of rooms
 * Args: [1] rooms, an array of Room structs
 *	[2] count, the number of Room structs in the array
 * pre: all rooms should be initialized
 * post: the rooms will have random connections such that the graph is full:
 * 	that is, every room has a valid number of connections to other rooms,
 * 	between MIN_CONNECTIONS and MAX_CONNECTIONS, inclusive
 * ret: nont
 *
 */
void addRandomConnection(struct Room* rooms, int count) {
	struct Room* x;
	struct Room* y;

	/*First, get a random room that can still add a connection */
	while(1) {
		x = getRandomRoom(rooms, count);
		
		if(canAddConnectionFrom(x) == 1 ) {
			break;
		}
	}

	/*Second, get a random room, different from the first one, that
 * 	can add a connection */
	do {
		y = getRandomRoom(rooms, count);

	} while(canAddConnectionFrom(y) == 0 || isSameRoom(x, y) == 1);

	/*Now that we have two random rooms, connect them if they haven't been connected before*/
	if( unconnected(x, y) && unconnected(y, x) ) {
		connectRoom(x, y);
		connectRoom(y, x);
	}

}

/*Initializes a room struct to valid starting values */
void initRoom(struct Room *room) {
		int i;

		room->name = NULL;
		room->type = -1;
		room->numConnections = 0;
		for(i = 0; i < MAX_CONNECTIONS; i++) {
			room->connections[i] = NULL;
		}
}


int main() {
	int used_names[NUM_NAMES];
	char* names[NUM_NAMES];
	int i;
	struct OneToOneNameMap map;
	struct Room rooms[NUM_ROOMS];
	int pid;
	int result;
	FILE* fd;
	char dirname[1000];
	char roomDescription[2000];
	memset(dirname, 0, sizeof(dirname)); /*zero out the directory name array*/
	memset(roomDescription, 0, sizeof(roomDescription));

	srand(time(NULL));

	/*Create an array of hard-coded room names*/
	names[0] = "FOYER";
	names[1] = "LONG_STAIRCASE";
	names[2] = "BASEMENT";
	names[3] = "DUNGEON";
	names[4] = "LIVING_ROOM";
	names[5] = "KITCHEN";
	names[6] = "DARK_ROOM";
	names[7] = "OPERATING_ROOM";
	names[8] = "DINING_ROOM";
	names[9] = "PRISON_CELL";

	/*Create array to store whether a name has been used*/
	for(i = 0; i < NUM_NAMES; i++) {
		used_names[i] = 0;
	}

	/*store these data structures in a OneToOneMap*/
	map.names = names;
	map.used_names = used_names; 
	map.size = NUM_NAMES;

	/* Create an array of 7 rooms, initialize each of them*/
	for(i = 0; i < NUM_ROOMS; i++) {
		initRoom(rooms + i);	
	}		

	/*Assign the rooms random names */
	assignRandomNames(rooms, NUM_ROOMS, &map);

	/*Assign the rooms random types*/
	assignRandomTypes(rooms, NUM_ROOMS);

	/*while the graph of Rooms isn't full, randomly connect a new pair of rooms
 * 		if it is valid to do so */
	while( graphIsFull(rooms, NUM_ROOMS) == 0) {
		addRandomConnection(rooms, NUM_ROOMS);
	}


	/*Make a directory to write the room files to
 * 	that is labeled with the pid of this program */
	pid = getpid();
	sprintf(dirname, "./chenhowa.rooms.%i", pid);

	result = mkdir(dirname, 0755);
	if(result != 0) {
		fprintf(stderr, "Directory creation failed!\n");
		return 1;
	}


	/*For each of the NUM_ROOM rooms, create and open a file named after the room
  		and write the contents of the room to a file within the new directory*/
	for(i = 0; i < NUM_ROOMS; i++) {
		memset(dirname, 0, sizeof(dirname)); /*zero out the directory name array*/
		sprintf(dirname, "./chenhowa.rooms.%i/%s", pid, rooms[i].name);
		fd = fopen(dirname, "w");

		if(!fd) {
			fprintf(stderr, "Could not open %s\n", dirname);
			return 1;
		}
		
		printRoom(fd, rooms + i);

		/*When finished, close the file */
		fclose(fd);
	}

	/*Done! */


	return 0;
}
