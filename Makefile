all:
	gcc -Wall -Wextra -Werror -std=c11 gc.c main.c -o main -lrt

test:
	gcc -Wall -Wextra -Werror -std=c11 -fsanitize=address gc.c main.c -o main -lrt
	./main --unit-tests

stress:
	gcc -Wall -Wextra -Werror -std=c11 gc.c main.c -o main -lrt
	./main --stress

clean:
	rm -f main