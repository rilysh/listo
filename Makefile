PROGRAM = listo
FLAGS = -Wall -Wextra -g

all:
	${CC} ${FLAGS} ${PROGRAM}.c -o ${PROGRAM}

clean:
	rm -f ${PROGRAM}
