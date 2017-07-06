#include "batch/print_util.h" 
#include "time_txn_allocation.h"
#include "time_SPSC_queue.h"
//#include "time_lock_table.h"
//#include "time_action_creation.h"

int main() {//int argc, char** argv) {
//  TimeSpscQueue::time_queue();
  TimeTxnAllocation::time_txn_allocation();
//  TimeActionCreation::time_action_creation();
//  TimeLockTable::time_lock_table();
  return 0;
}
