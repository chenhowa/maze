CC = gcc
CFLAGS = -Wall -pedantic

SRC_ROOM = chenhowa.buildrooms.c
OBJ_ROOM = chenhowa.buildrooms.o
HEADERS = 
SRC_AD = chenhowa.adventure.c
OBJ_AD = chenhowa.adventure.o

rooms: ${OBJ_ROOM} ${HEADERS}
	${CC} ${SRC_ROOM} -o chenhowa.buildrooms

${OBJ_ROOM}: ${SRC_ROOM}
	${CC} ${CFLAGS} -c $(@:.o=.c)

adventure: ${OBJ_AD} ${HEADERS}
	${CC} ${SRC_AD} -o chenhowa.adventure -lpthread

${OBJ_AD}: ${SRC_AD}
	${CC} ${CFLAGS} -c $(@:.o=.c)

debug: ${OBJ_ROOM}
	${CC} ${CFLAGS} -g ${SRC_ROOM} -o debug

clean: 
	rm -r *.o chenhowa.buildrooms chenhowa.adventure debug chenhowa.rooms.* currentTime.txt *~
