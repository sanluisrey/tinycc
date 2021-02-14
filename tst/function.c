#include "test.h"


int ret3() {
	return 3;
	return 5;
}

int main() {
	ASSERT(3, ret3());
	return 0;
}
