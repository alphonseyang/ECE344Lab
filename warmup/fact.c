#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int factorial(int value){
	if(value != 1){
		value = value * factorial(value-1);
	}
	return value;
}


int
main(int argc, char *argv[])
{
	int converted = atoi(argv[1]);
	float converted_f = atof(argv[1]);
	if(argc != 2 || converted == 0 || ceilf(converted_f) != converted_f){
		printf("Huh?\n");
	} else if(converted > 12){
		printf("Overflow\n");
	} else{
		int result = factorial(converted);
		printf("%d\n", result);
	}
	return 0;
}

