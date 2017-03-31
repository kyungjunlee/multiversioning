#ifndef BATCH_ACTION_FACTORY_IMPL_
#define BATCH_ACTION_FACTORY_IMPL_

#include "batch/txn_factory.h"
#include "test/test_action.h"

#include <random>

template <class ActionClass>
std::unordered_set<unsigned int> 
ActionFactory<ActionClass>::get_disjoint_set_of_random_numbers(
    unsigned int from, 
    unsigned int to, 
    unsigned int how_many,
    std::unordered_set<unsigned int> constraining_set) {
  unsigned int range = from - to + 1;
  unsigned int constraining_nums_in_set = 0;
  for (auto& num : constraining_set) {
    if (num >= from && num <= to) constraining_nums_in_set ++; 
  }
  assert(how_many + constraining_nums_in_set <= range);

  std::random_device rand_gen;
  std::uniform_int_distribution<unsigned int> distro(from, to);
  std::unordered_set<unsigned int> result;
 
  if (how_many + constraining_nums_in_set > 0.5 * range) {
    // initialize with all of the numbers not in constraining set.
    for (unsigned int i = from; i <= to; i++) {
      if (constraining_set.find(i) == constraining_set.end()) {
        result.insert(i);
      }
    }

    // reject.
    while (result.size() > how_many) {
      result.erase(distro(rand_gen));
    }
  } else {
    while (result.size() < how_many) {
      auto i = distro(rand_gen);
      if (constraining_set.find(i) == constraining_set.end()) {
        result.insert(i);
      }
    }
  } 
  
  return result;
};

template <class ActionClass>
unsigned int ActionFactory<ActionClass>::get_lock_number(
    LockDistributionConfig conf) {
  std::random_device dev;
  std::normal_distribution<double> distro(
      conf.average_num_locks, conf.std_dev_of_num_locks);

  if (distro.mean() == 0) return 0;

  // negative numbers do not make sense.
  double result;
  while ((result = distro(dev)) < 0);

  assert(result >= 0);
  return static_cast<unsigned int>(result);
}

template <class ActionClass>
bool ActionFactory<ActionClass>::lock_distro_config_is_valid(
    LockDistributionConfig conf) {
  if (conf.low_record < 0 || 
      conf.low_record > conf.high_record ||
      conf.high_record < 0 ||
      conf.average_num_locks < 0 ||
      conf.std_dev_of_num_locks < 0) {
    return false;
  }

  return true;
};
// TODO:
//    Uncomment this and add additional checks after hotsets
//    are added.
//
//  // check if hotsets and non-hotsets overlap
//  auto config_overlap = [](
//      LockDistributionConfig hotset_conf,
//      LockDistributionConfig config){
//
//    // Overlap with config low in hotset.
//    if (config.low_record > hotset_config.low_record &&
//        config.low_record < hotset_config.high_record) {
//      return true;
//    }
//
//    // Overlap with config high in hotset
//    if (config.high_record < hotset_config.high_record &&
//        config.high_record > hotset_config.low_record) {
//      return true;
//    }
//
//    // Overlap with hotset within config
//    if (hotset_config.low_record > config.low_record &&
//        hotset_config.low_record < config.high_record) {
//      return true;
//    }
//
//    return false;
//  }; 
//
//  assert(!config_overlap(spec.writes, spec.hotset_writes));
//  assert(!config_overlap(spec.reads, spec.hotset_reads));
//};

template <class ActionClass>
std::vector<std::unique_ptr<IBatchAction>>
ActionFactory<ActionClass>::generate_actions (
    ActionSpecification spec,
    unsigned int number_of_actions) {
  assert(
      lock_distro_config_is_valid(spec.reads) &&
      lock_distro_config_is_valid(spec.writes));
  
  std::vector<std::unique_ptr<IBatchAction>> res;
  for (unsigned int i = 0; i < number_of_actions; i++) {
    auto read_set = get_disjoint_set_of_random_numbers(
        spec.reads.low_record,
        spec.reads.high_record,
        get_lock_number(spec.reads));

    auto write_set = get_disjoint_set_of_random_numbers(
        spec.writes.low_record,
        spec.writes.high_record,
        get_lock_number(spec.writes),
        read_set);

    // construct the action
    std::unique_ptr<IBatchAction> act = std::make_unique<ActionClass>(new TestTxn());
    for (auto& key : read_set) act->add_read_key(key);
    for (auto& key : write_set) act->add_write_key(key);

    res.push_back(std::move(act));
  }

  return res;
}

#endif // BATCH_ACTION_FACTORY_IMPL_
