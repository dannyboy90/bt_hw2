#include <iostream>
using namespace std;
#define FOR_ITERARIONS 4

int forLoopTestFunc(int iterations) {
	int a;
	for (int i = 0; i < iterations; i++) {
		//	cout<<"Iteration number: "<<it+1<<endl;
		a += 1;
	}
	return 0;
}
int main() {
	int a = FOR_ITERARIONS;
	int b = FOR_ITERARIONS;
	int c = FOR_ITERARIONS;
	// forLoopTestFunc(1);
	// forLoopTestFunc(2);
	// forLoopTestFunc(2);
	while (a > 0) {
		if (a == FOR_ITERARIONS / 2) break;
		a--;
	}
	cout << "loop_test_simple done!" << endl;
	return 0;
}