#ifndef PACKING_H_
#define PACKING_H_

#include <batch/container.h>

#include <vector>
#include <unordered_set>

/**
 * Pseudo - static class containing logic for creation of packings. 
 *
 * This is inherently single-threaded and no synchronization if provided.
 **/
class Packer {
public:
  typedef uint64_t RecordKey;
  typedef std::unordered_set<RecordKey> RecordSet;
  typedef std::unique_ptr<BatchAction> Action_upt;
  typedef std::vector<Action_upt> ActionUptVector;
private:
  Packer();

  static bool txn_conflicts(
      BatchAction* t, 
      RecordSet* ex_locks_in_packing, 
      RecordSet* sh_locks_in_packing); 

public:
  static ActionUptVector get_packing(Container* c);
};

#endif // PACKING_H_
