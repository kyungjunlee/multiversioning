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
  LockDistributionConfig read_conf;
  LockDistributionConfig write_conf;
  std::unordered_set<unsigned int> read_set;
  std::unordered_set<unsigned int> write_set;
  std::random_device rand_gen;
  std::normal_distribution<double> read_num_distro;
  std::uniform_int_distribution<unsigned int> read_distro;
  std::normal_distribution<double> write_num_distro;
  std::uniform_int_distribution<unsigned int> write_distro;

  void initialize_from_spec(ActionSpecification spec);
  // runs in expected polynomial time.
  //
  // to and from are both inclusive.
  void fill_disjoint_set_of_random_numbers(
      unsigned int how_many_numbers,
      std::uniform_int_distribution<unsigned int>& distro,
      std::unordered_set<unsigned int>& working_set,
      const std::unordered_set<unsigned int>& constraining_set = {});
  unsigned int get_nonzero_integer(std::normal_distribution<double>& distro);

public:
  ActionFactory(ActionSpecification spec);

  bool lock_distro_config_is_valid(LockDistributionConfig spec);
  std::vector<std::unique_ptr<IBatchAction>> generate_actions(
      unsigned int number_of_actions);
  void initialize_txn_to_random_values(IBatchAction* act);
  void initialize_txn_to_random_values(std::unique_ptr<IBatchAction>& act);
  void set_action_spec(ActionSpecification spec);
};

#include "batch/txn_factory_impl.h"

#endif // BATCH_ACTION_FACTORY_H_
