struct A {
	long l;
	short s;
	int i;
	char c;
};

struct B {
	int j;
	struct A a[4];
	struct A* pa;
	struct B* pb;
};

struct A a;
struct B b;

int main(int argc, char** argv)
{
	a.i = 0;
	b.a[0].i = 0;
	return a.i + b.a[0].i;
}
