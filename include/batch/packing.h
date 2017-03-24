#ifndef PACKING_H_
#define PACKING_H_

#include <batch/container.h>
#include <batch/batch_action_interface.h>

#include <vector>
#include <unordered_set>

/**
 * Pseudo - static class containing logic for creation of packings. 
 *
 * This is inherently single-threaded and no synchronization if provided.
 **/
class Packer {
private:
  typedef IBatchAction::RecordKeySet RecordKeySet;
  typedef Container::BatchActions BatchActions;

  Packer();

  static bool txn_conflicts(
      IBatchAction* t, 
      RecordKeySet* ex_locks_in_packing, 
      RecordKeySet* sh_locks_in_packing); 

public:
  static BatchActions get_packing(Container* c);
};

#endif // PACKING_H_
