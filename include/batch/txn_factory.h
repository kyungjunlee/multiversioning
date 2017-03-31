#ifndef BATCH_ACTION_FACTORY_H_
#define BATCH_ACTION_FACTORY_H_

#include "batch/batch_action_interface.h"

#include <memory>
#include <vector>

// Lock Distribution Config
//
//    Used to specify the distribution of locks within a write/read set.
//    We assume only one table and assignment of locks in a uniformly random
//    fashion within the interval [low_record, high_record] (both inclusive).
//
//    The number of locks requested is specified by its average and std dev.
//    The exact number is computed using a discretized normal distribution.
struct LockDistributionConfig {
  unsigned int low_record;
  unsigned int high_record;
  unsigned int average_num_locks;
  unsigned int std_dev_of_num_locks;
};

//  Action Specification
//
//      Used to specify the write and read set requirements of an action
//      via Lock Distribution Configs. 
//
//      For now we do not support a combination of hot and non-hot sets.
//
// TODO:
//    Hotsets.
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
  static bool lock_distro_config_is_valid(LockDistributionConfig spec);
public:
  static std::vector<std::unique_ptr<IBatchAction>> generate_actions(
      ActionSpecification spec,
      unsigned int number_of_actions);
};

#include "batch/txn_factory_impl.h"

#endif // BATCH_ACTION_FACTORY_H_
