
/*

This tool collects statistics about loops in a given binary (and shared objects) and output them to a file loop-count.csv
Definitions:
A background edge traversal counts as a loop iteration.
A forward edge (NOT to a follow through BBL of a loop condition BBL) traversal counts as a loop invocation

We do not count invocations where a loop was escaped during its body execution.

*/

#include <string.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include "pin.H"

#define DEBUG(X)

/* #define DEBUG(x)  \
 do {             \
  std::cout << x; \
 } while (0)
 */
ofstream outFile;

// Holds information for a simple loop containing a SINGLE bbl
typedef struct LoopCount {
	ADDRINT _target_address;
	UINT64 _count_seen;                  // total number of times the loop’s backward edge was executed
	UINT64 _count_seen_curr_invocation;  // total number of times the loop’s backward edge was executed in the current
	                                     // invocation
	UINT64 _count_seen_last_invocation;  // total number of times the loop’s backward edge was executed in the last
	                                     // invocation
	UINT64 _count_loop_invoked;          //	number of times the loop was invoked
	UINT64 _mean_taken;                  //	average number of iterations taken for the loop invocations
	INT64 _diff_count;  //	number of times that two successive loop invocations took a different number of iterations

	string _rtn_name;
	ADDRINT _rtn_address;
	string _bbl_image_name;

	INS _head;
	INS _tail;
	ADDRINT _head_address;
	ADDRINT _tail_address;
	ADDRINT _tail_target_address;
	ADDRINT _tailNextAddr;
	UINT64 _rtnCount;
	UINT64 _icount;

	struct LoopCount *_next;
	bool operator<(const LoopCount &str) const { return (_count_seen > str._count_seen); }
	bool _ends_with_backward_edge;
	ADDRINT _slave_address;

} LOOP_COUNT;
// Linked list of instruction counts for each routine
LOOP_COUNT *LoopList = 0;

INT32 LoopListSize = 0;

const char *StripPath(const char *path) {
	const char *file = strrchr(path, '/');
	if (file)
		return file + 1;
	else
		return path;
}

map<ADDRINT, LOOP_COUNT *> BblMap;            // Mapping from address to corisponding BBL struct
map<ADDRINT, ADDRINT> LoopCondition2BodyMap;  // Map loop condition (last bbl) to body bbl
map<ADDRINT, int> RtnCount;
VOID docount2(UINT64 *counter) { (*counter)++; }
VOID Routine(RTN rtn, VOID *v) {
	std::pair<map<ADDRINT, int>::iterator, bool> ret;
	ret = RtnCount.insert(pair<ADDRINT, int>(RTN_Address(rtn), 0));
	
	if (ret.second == false) {
		cout << "Revisiting the same routine?!" << endl;
	} else {
		RTN_Open(rtn);

		// Insert a call at the entry point of a routine to increment the call count
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)docount2, IARG_PTR, &(ret.first->second), IARG_END);
		RTN_Close(rtn);
	}
}

// This function is called before every instruction is executed
VOID docount(VOID *instPtr, INT32 isTaken, VOID *fallthroughAddr, VOID *takenAddr, LOOP_COUNT *lc) {
	std::map<ADDRINT, ADDRINT>::iterator C2B_it;
	// cout<<"Reached insert call for: "<<hex<<lc->_head_address<< "for routine: "<<lc->_rtn_name<<" lc->_tail_target_address = "<<lc->_tail_target_address<<endl; 
	if (lc->_ends_with_backward_edge) {  //	Backword edge
		lc->_count_seen++;
		lc->_count_seen_curr_invocation++;
		C2B_it = LoopCondition2BodyMap.find(lc->_tail_target_address);
		if (C2B_it == LoopCondition2BodyMap.end()) {
			LoopCondition2BodyMap[lc->_tail_target_address] = lc->_head_address;
			DEBUG("Binding Loop Condition BBL @ " << hex << lc->_tail_target_address << " with Body BBL @ "
			                                      << lc->_head_address << "LoopCondition2BodyMap size is now "
			                                      << LoopCondition2BodyMap.size() << endl);
		}
	} else if (isTaken) {  //	if branch is taken and we are escaping the loop
		C2B_it = LoopCondition2BodyMap.find(lc->_head_address);
		if (C2B_it != LoopCondition2BodyMap.end()) {
			ADDRINT body_address = C2B_it->second;
			std::map<ADDRINT, LOOP_COUNT *>::iterator it;
			it = BblMap.find(body_address);
			if (it != BblMap.end()) {  //	if the loop body was executed
				LOOP_COUNT *lc_body = it->second;
				DEBUG("DiffCount = " << dec << lc->_diff_count << endl);
				if (lc->_count_loop_invoked != 0 &&
				    lc_body->_count_seen_last_invocation != lc_body->_count_seen_curr_invocation) {
					lc->_diff_count++;
				}
				lc_body->_count_seen_last_invocation = lc_body->_count_seen_curr_invocation;
				lc_body->_count_seen_curr_invocation = 0;
			}
			lc->_count_loop_invoked++;  //	increases only master's
		}
	}
}
// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID *v) {
	RTN currRtn = TRACE_Rtn(trace);
	if (RTN_Valid(currRtn)) {
		for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
			string rtn_name = RTN_Name(currRtn);
			string img_name = StripPath(IMG_Name(SEC_Img(RTN_Sec(currRtn))).c_str());

			INS bbl_tail = BBL_InsTail(bbl);
			if (INS_IsDirectBranch(bbl_tail) &&  rtn_name.find("pthread") == string::npos) { //Doesn't work in pthread libs
				LOOP_COUNT *lc = new LOOP_COUNT;
				{
					lc->_head = BBL_InsHead(bbl);
					lc->_tail = bbl_tail;
					lc->_target_address = INS_Address(lc->_head);
					lc->_count_seen = 0;
					lc->_count_loop_invoked = 0;
					lc->_mean_taken = 0;
					lc->_diff_count = 0;  //	to offset the first time where there's nothing to compare with last time
					lc->_rtn_name = rtn_name;
					lc->_rtn_address = RTN_Address(currRtn);
					lc->_bbl_image_name = img_name;
					lc->_head_address = INS_Address(lc->_head);
					lc->_tail_address = INS_Address(bbl_tail);
					lc->_tail_target_address = INS_DirectBranchOrCallTargetAddress(lc->_tail);
					lc->_tailNextAddr = INS_NextAddress(lc->_tail);
					lc->_count_seen_curr_invocation = 0;
					lc->_count_seen_last_invocation = 0;
					lc->_rtnCount = 0;
					lc->_icount = 0;
					int offset = -(lc->_tail_address - lc->_tail_target_address);
					if (offset < 0) {
						lc->_ends_with_backward_edge = true;
					} else {
						lc->_ends_with_backward_edge = false;
					}
				}
				std::pair<map<ADDRINT, LOOP_COUNT *>::iterator, bool> ret;

				ret = BblMap.insert(pair<ADDRINT, LOOP_COUNT *>(lc->_head_address, lc));

				if (ret.second == false) {
					// DEBUG("Old LoopBBL encountered (inside routine: "
					//       << lc->_rtn_name << ") H: 0x" << hex << lc->_head_address << " T: 0x" << lc->_tail_address << "-> 0x"
					//       << lc->_tail_target_address << " _ends_with_backward_edge=" << lc->_ends_with_backward_edge << endl);
					lc = ret.first->second;
				} else {
					// DEBUG("New LoopBBL encountered (inside routine: "
					//       << lc->_rtn_name << ") H: 0x" << hex << lc->_head_address << " T: 0x" << lc->_tail_address << "-> 0x"
					//       << lc->_tail_target_address << " _ends_with_backward_edge=" << lc->_ends_with_backward_edge << endl);
				}
				
				INS_InsertCall(bbl_tail, IPOINT_BEFORE, (AFUNPTR)docount, IARG_INST_PTR, IARG_BRANCH_TAKEN,
				               IARG_FALLTHROUGH_ADDR, IARG_BRANCH_TARGET_ADDR, IARG_PTR, lc, IARG_END);
			}
		}
	}
}

// This function is called when the application exits
// It prints the name and count for each procedure
VOID Fini(INT32 code, VOID *v) {
	LOOP_COUNT *CondBBL;
	LOOP_COUNT *BodyBBL;
	std::vector<LOOP_COUNT> vec;
	map<ADDRINT, ADDRINT>::iterator it;
	for (it = LoopCondition2BodyMap.begin(); it != LoopCondition2BodyMap.end(); ++it) {
		if (BblMap.find(it->first) != BblMap.end()) {
			CondBBL = BblMap[it->first];
			BodyBBL = BblMap[it->second];
			CondBBL->_count_seen = BodyBBL->_count_seen;

			if (CondBBL->_count_loop_invoked != 0) {
				CondBBL->_mean_taken = CondBBL->_count_seen / CondBBL->_count_loop_invoked;
			}
			CondBBL->_rtnCount = RtnCount[CondBBL->_rtn_address];
			vec.push_back(*CondBBL);
		}
	}
	std::sort(vec.begin(), vec.end());

	//	0x<image 1 address>, <image 1 name> , 0x<routine 1 address>, <routine 1 name> , <instructions count of routine
	//	1 >
	// 0x <loop 1 target address>, <loop 1 CountSeen>, <loop 1 CountLoopInvoked>, <loop 1 MeanTaken>, <loop 1
	// DiffCount>, <loop 1 routine name>, 0x <loop 1 routine address> , <loop 1 routine instructions count>
	outFile << "Loop Target Address     ,"
	        << " "
	        << "CountSeen        ,"
	        << " "
	        << "CountLoopInvoked   ,"
	        << " "
	        << "MeanTaken   ,"
	        << " "
	        << "DiffCount   ,"
	        << " "
	        << "Routine Name      ,"
	        << " "
	        << "Routine Address      ,"
	        << " "
	        //  <<   "Calls             ," << " "
	        << "Routine Invocations Count" << endl;
	for (std::vector<LOOP_COUNT>::iterator it = vec.begin(); it != vec.end(); ++it) {
		LOOP_COUNT *lc = &*it;
		outFile << "0x" << hex << lc->_head_address << dec << ", " << lc->_count_seen << ", " << lc->_count_loop_invoked
		        << ", " << lc->_mean_taken << ", " << lc->_diff_count << ", " << lc->_rtn_name << ", "
		        << "0x" << hex << lc->_rtn_address << dec << ", " << lc->_rtnCount << endl;
	}
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
	cerr << "This Pintool counts the number of times a routine is executed" << endl;
	cerr << "and the number of instructions executed in a routine" << endl;
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[]) {
	// Initialize symbol table code, needed for rtn instrumentation
	PIN_InitSymbols();

	outFile.open("/mnt/hgfs/HW/hw2/src/loop-count.csv");

	// Initialize pin
	if (PIN_Init(argc, argv)) return Usage();

	// Register Routine to be called to instrument rtn
	RTN_AddInstrumentFunction(Routine, 0);

	// Register Instruction to be called to instrument instructions
	TRACE_AddInstrumentFunction(Trace, 0);

	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}
