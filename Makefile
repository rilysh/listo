PROGRAM = listo
FLAGS = -Wall -Wextra -O2 -s

all:
	${CC} ${FLAGS} ${PROGRAM}.c -o ${PROGRAM}

clean:
	rm -f ${PROGRAM}
