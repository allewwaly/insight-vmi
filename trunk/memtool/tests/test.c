#include <stdio.h>
//#include <stdlib.h>

//#include <linux/calc64.h>
#include <linux/kernel.h>
#include <linux/types.h>
//#include <linux/time.h>
//#include <linux/timex.h>
#include <asm/param.h>                  /* for HZ */
//#include <linux/jiffies.h>

int someFunc(int param1, char* param2)
{
    int i;
    for (i = 0; i < 10; i++)
        printf("%d: %d\n", i, param1);
    return 0;
}

// Basic types
char some_char;
unsigned char some_uchar;
int some_int;
unsigned int some_unit;
long some_long;
unsigned long some_ulong;
float some_float;
double some_double;
const int some_const_int = 5;
int *some_pint;

typedef int int_typedef;
int_typedef some_int_typedef_inst;

struct {
	int struct_int;
	char struct_char;
	long struct_long;
} some_struct_inst;


struct struct_def {
	int structdef_int;
	char structdef_char;
	long structdef_long;
};

typedef struct {
	float tstruct_float;
	double tstruct_double;
} struct_typedef;

struct_typedef some_struct_typedef_inst;

typedef struct _double_name {
	int dstruct_int;
} double_name;

struct _double_name some_struct_double_name;
double_name some_double_name;

struct nested_struct {
	long member_long;
	struct struct_def nested_struct_def;
	struct _double_name nested_double_name;
};


int int_array10[10];
int int_array3[] = {1, 2, 3};
char* string = "test";
char char_array[] = "test";

enum SomeEnum {
	seOne,
	seTwo,
	seThree
} some_enum_inst;


int main(int argc, char **argv) {

	printf("HZ: %d\n", HZ);

	return 0;
}

