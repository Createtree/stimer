OUTPUT_PATH = output

out: test.o stimer.o
	gcc -g ${OUTPUT_PATH}/stimer.o ${OUTPUT_PATH}/test.o -o ${OUTPUT_PATH}/out

stimer.o: stimer.c
	gcc -o ${OUTPUT_PATH}/stimer.o -c -g stimer.c

test.o: test.c
	gcc -o ${OUTPUT_PATH}/test.o -c -g test.c

clean:
	rm *.o *.exe