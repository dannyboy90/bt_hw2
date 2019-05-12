#include <iostream>
using namespace std;
#define FOR_ITERARIONS 3

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
	int c=1;
	forLoopTestFunc(1);
	forLoopTestFunc(2);
	forLoopTestFunc(3);
	while (a > 0) {
		a--;
		c++;
	}
	// while (b > 0) {
	// 	b--;
	// 	c++;
	// }
	// for(int i=0;i<FOR_ITERARIONS;i++){
	// 	c++;
	// }
	// forLoopTestFunc();
	return 0;
}