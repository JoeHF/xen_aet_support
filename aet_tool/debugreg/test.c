#include <stdio.h>

int a[10];
int main() {
	int d;
	printf("a:%p %lu\n", a, (unsigned long)(&a));
	scanf("%d", &d);
	a[0] = 1;
	return 0;
}
