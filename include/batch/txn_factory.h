#ifndef BATCH_ACTION_FACTORY_H_
#define BATCH_ACTION_FACTORY_H_

#include "batch/batch_action_interface.h"

#include <memory>
#include <vector>

// Assumption: one table only.
struct LockDistributionConfig {
  unsigned int low_record;
  unsigned int high_record;
  unsigned int average_num_locks;
  unsigned int std_dev_of_num_locks;
};

// note: hotsets and non-hotsets musn't overlap
// note: no overflow. We cannot ask for more 
// note: only support one table.
// TODO:
//  Hotesets
struct ActionSpecification {
  LockDistributionConfig writes;
//  LockDistributionConfig hotset_writes;
  LockDistributionConfig reads;
//  LockDistributionConfig hotset_reads;
};

template <class ActionClass>
class ActionFactory {
private:
  // runs in expected polynomial time.
  //
  // to and from are both inclusive.
  static std::unordered_set<unsigned int> get_disjoint_set_of_random_numbers(
      unsigned int from,
      unsigned int to,
      unsigned int how_many, 
      std::unordered_set<unsigned int> constraining_set = {});
  static unsigned int get_lock_number(LockDistributionConfig conf);
//  static bool configs_passed_are_valid(ActionSpecification spec);
public:
  static std::vector<std::unique_ptr<IBatchAction>> generate_actions(
      ActionSpecification spec,
      unsigned int number_of_actions);
};

#include "batch/txn_factory_impl.h"

#endif // BATCH_ACTION_FACTORY_H_
