struct A {
	long l;
	short s;
	int i;
	char c;
};

struct B {
	int j;
	struct A a;
	struct A array[4];
	struct A* pa;
	struct B* pb;
	union {
		int nested_i;
		float nested_f;
		struct A nested_a;
	};
};

struct A a;
struct B b;

int main(int argc, char** argv)
{
	a.i = 0;
	b.array[0].i = 0;
	return a.i + b.array[0].i;
}
