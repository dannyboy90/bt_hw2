
//
// This tool counts the number of times a routine is executed and
// the number of instructions executed in a routine
//

#include <string.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include "pin.H"
#define DEBUG(x)  \
	do {             \
		std::cout << x; \
	} while (0)

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
	UINT64 _diff_count;  //	number of times that two successive loop invocations took a different number of iterations

	string _rtn_name;
	ADDRINT _rtn_address;
	string _bbl_image_name;

	INS _head;
	INS _tail;
	ADDRINT _head_address;
	ADDRINT _tail_address;
	ADDRINT _tail_target_address;
	ADDRINT _fallthroughAddr;
	UINT64 _rtnCount;
	UINT64 _icount;

	struct LoopCount *_next;
	bool operator<(const LoopCount &str) const { return (_count_seen > str._count_seen); }
	bool _ends_with_backward_edge;
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

map<ADDRINT, LOOP_COUNT *> LoopMap;

// This function is called before every instruction is executed
VOID docount(VOID *instPtr, INT32 isTaken, VOID *fallthroughAddr, VOID *takenAddr, LOOP_COUNT *lc) {
	int offset = -(lc->_tail_address - lc->_tail_target_address);

	if (offset < 0) {    //	Backword edge
		lc->_count_seen++;  //	increases only slave's
		lc->_count_seen_curr_invocation++;
	} else if (isTaken != 0) {
		std::map<ADDRINT, LOOP_COUNT *>::iterator it;
		it = LoopMap.find(lc->_fallthroughAddr);
		if (it != LoopMap.end()) {
			LOOP_COUNT *lc_slave = it->second;
			if (lc_slave->_count_seen_last_invocation != lc_slave->_count_seen_curr_invocation) {
				lc_slave->_diff_count++;
			}
			lc_slave->_count_seen_last_invocation = lc_slave->_count_seen_curr_invocation;
			lc_slave->_count_seen_curr_invocation = 0;
		}
		lc->_count_loop_invoked++;  //	increases only master's
	}
	DEBUG(hex << "head=" << lc->_head_address << " tail=" << lc->_tail_address
	          << " tail target address=" << lc->_tail_target_address
	          << " _count_seen_curr_invocation=" << lc->_count_seen_curr_invocation << "_fallthroughAddr"
	          << lc->_fallthroughAddr << " offset=" << dec << offset << " isTaken=" << isTaken << endl);
}
VOID docount2(UINT64 *counter) { (*counter)++; }
// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID *v) {
	RTN currRtn = TRACE_Rtn(trace);
	if (RTN_Valid(currRtn)) {
		for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
			string rtn_name = RTN_Name(currRtn);
			string img_name = StripPath(IMG_Name(SEC_Img(RTN_Sec(currRtn))).c_str());

			INS bbl_tail = BBL_InsTail(bbl);
			if (INS_IsDirectBranch(bbl_tail) && (rtn_name.compare("main") == 0||rtn_name.find("forLoopTestFunc") != string::npos) /* &&
			    img_name.find("loop_test") != string::npos */) {
				LOOP_COUNT *lc = new LOOP_COUNT;
				{
					lc->_target_address = INS_Address(BBL_InsHead(bbl));
					lc->_count_seen = 0;
					lc->_count_loop_invoked = 0;
					lc->_mean_taken = 0;
					lc->_diff_count = -1;  //	to offset the first time where there's nothing to compare with last time
					lc->_rtn_name = RTN_Name(currRtn);
					lc->_rtn_address = RTN_Address(currRtn);
					lc->_bbl_image_name = StripPath(IMG_Name(SEC_Img(RTN_Sec(currRtn))).c_str());
					lc->_head = BBL_InsHead(bbl);
					lc->_tail = BBL_InsTail(bbl);
					lc->_head_address = INS_Address(BBL_InsHead(bbl));
					lc->_tail_address = INS_Address(BBL_InsTail(bbl));
					lc->_tail_target_address = INS_DirectBranchOrCallTargetAddress(lc->_tail);
					lc->_fallthroughAddr = INS_NextAddress(lc->_tail);
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

				ret = LoopMap.insert(pair<ADDRINT, LOOP_COUNT *>(lc->_head_address, lc));

				if (ret.second == false) {
					DEBUG("Old LoopBBL encountered (inside routine: "
					      << lc->_rtn_name << ") Head: 0x" << hex << lc->_head_address << " Tail: 0x" << lc->_tail_address
					      << "-> 0x" << lc->_tail_target_address << " _ends_with_backward_edge=" << lc->_ends_with_backward_edge
					      << endl);
					lc = ret.first->second;
				} else {
					DEBUG("New LoopBBL encountered (inside routine: "
					      << lc->_rtn_name << ") Head: 0x" << hex << lc->_head_address << " Tail: 0x" << lc->_tail_address
					      << "-> 0x" << lc->_tail_target_address << " _ends_with_backward_edge=" << lc->_ends_with_backward_edge
					      << endl);
				}
				INS_InsertCall(bbl_tail, IPOINT_BEFORE, (AFUNPTR)docount, IARG_INST_PTR, IARG_BRANCH_TAKEN,
				               IARG_FALLTHROUGH_ADDR, IARG_BRANCH_TARGET_ADDR, IARG_PTR, lc, IARG_END);
				// RTN_Open(currRtn);
				// // Insert a call at the entry point of a routine to increment the call count
				// RTN_InsertCall(currRtn, IPOINT_BEFORE, (AFUNPTR)docount2, IARG_PTR, &(lc->_rtnCount), IARG_END);
				// RTN_Close(currRtn);
			}
		}
	}
}

// This function is called when the application exits
// It prints the name and count for each procedure
VOID Fini(INT32 code, VOID *v) {
	cout << "---Fini---" << endl;
	LOOP_COUNT *currBBL;
	std::vector<LOOP_COUNT> vec;
	map<ADDRINT, LOOP_COUNT *>::iterator it;
	cout << "Found " << LoopMap.size() << " BBLS" << endl;
	for (it = LoopMap.begin(); it != LoopMap.end(); ++it) {
		//	Filter out all the unrelated routines, each tested routine must contain TestFunc in its name!
		currBBL = it->second;
		if (currBBL->_ends_with_backward_edge == false) {
			DEBUG("Found master bbl with head @ " << hex << currBBL->_head_address << endl);
			LoopMap[currBBL->_fallthroughAddr]->_count_loop_invoked += currBBL->_count_loop_invoked;
		} else if (currBBL->_count_seen > 0) {
			currBBL->_mean_taken = currBBL->_count_seen / currBBL->_count_loop_invoked;
			vec.push_back(*currBBL);
		}
	}
	cout << "Found " << vec.size() << " loops" << endl;
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
	        << "Invocations Count" << endl;
	for (std::vector<LOOP_COUNT>::iterator it = vec.begin(); it != vec.end(); ++it) {
		LOOP_COUNT *lc = &*it;
		outFile << "0x" << hex << lc->_tail_target_address << dec << ", " << lc->_count_seen << ", "
		        << lc->_count_loop_invoked << ", " << lc->_mean_taken << ", " << lc->_diff_count << ", " << lc->_rtn_name
		        << ", "
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
	// RTN_AddInstrumentFunction(Routine, 0);

	// Register Instruction to be called to instrument instructions
	TRACE_AddInstrumentFunction(Trace, 0);

	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}
