#include <iostream>
#include "loop_testLib.hpp"

using namespace std;
int forLoop(int i){
	for(int it=1; it<=i; it++){
      cout<<"Iteration number: "<<it<<endl;
   }
   return 0;
}