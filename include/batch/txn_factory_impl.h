#ifndef BATCH_ACTION_FACTORY_IMPL_
#define BATCH_ACTION_FACTORY_IMPL_

#include "batch/txn_factory.h"
#include "test/test_action.h"

#include <random>
#include <cassert>

template <class ActionClass>
ActionFactory<ActionClass>::ActionFactory(ActionSpecification act_spec)
{
  initialize_from_spec(act_spec);
  lock_distro_config_is_valid(read_conf);
  lock_distro_config_is_valid(write_conf);
};

template <class ActionClass>
void ActionFactory<ActionClass>::initialize_from_spec(ActionSpecification spec) {
  read_conf = spec.reads;
  write_conf = spec.writes;

  auto init_norm_distr = [](
      std::normal_distribution<double>& distro,
      LockDistributionConfig conf) {
    distro = std::normal_distribution<double>(
        conf.average_num_locks, conf.std_dev_of_num_locks);
  };
  auto init_int_distr = [](
      std::uniform_int_distribution<unsigned int>& distro,
      LockDistributionConfig conf) {
    distro = std::uniform_int_distribution<unsigned int>(
        conf.low_record, conf.high_record);
  };

  init_norm_distr(read_num_distro, read_conf);
  init_norm_distr(write_num_distro, write_conf);

  init_int_distr(read_distro, read_conf);
  init_int_distr(write_distro, write_conf);
};

template <class ActionClass>
void ActionFactory<ActionClass>::fill_disjoint_set_of_random_numbers(
    unsigned int how_many_numbers,
    std::uniform_int_distribution<unsigned int>& distro,
    std::unordered_set<unsigned int>& working_set,
    const std::unordered_set<unsigned int>& constraining_set) {
  // # of ints in constrianing set within sampling range
  unsigned int constraining_nums_in_set = 0;
  for (auto& num : constraining_set) {
    if (num >= distro.max() && num <= distro.min()) {
      constraining_nums_in_set ++; 
    }
  }

  // there exist enough numbers to sample from.
  unsigned int sampling_space_size = distro.max() - distro.min() + 1;
  assert(
      how_many_numbers + constraining_nums_in_set <= sampling_space_size);

  working_set.clear();
  if (how_many_numbers + constraining_nums_in_set > 0.5 * sampling_space_size) {
    // rejection sampling. Fill all legal possibilities.
    for (unsigned int i = distro.min(); i <= distro.max(); i++) {
      if (constraining_set.find(i) == constraining_set.end()) {
        working_set.insert(i);
      }
    }

    // reject.
    while (working_set.size() > how_many_numbers) {
      working_set.erase(distro(rand_gen));
    }

    return;
  }

  // regular sampling
  while (working_set.size() < how_many_numbers) {
    auto i = distro(rand_gen);
    if (constraining_set.find(i) == constraining_set.end()) {
      working_set.insert(i);
    }
  }
};

template <class ActionClass>
unsigned int ActionFactory<ActionClass>::get_nonzero_integer(
    std::normal_distribution<double>& distro) {
  if (distro.mean() == 0) return 0;

  // negative numbers do not make sense.
  double result;
  while ((result = distro(rand_gen)) < 0);

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

template <class ActionClass>
std::vector<std::unique_ptr<IBatchAction>>
ActionFactory<ActionClass>::generate_actions(
    unsigned int number_of_actions) {

  std::vector<std::unique_ptr<IBatchAction>> actions;
  for (unsigned int i = 0; i < number_of_actions; i++) {
    std::unique_ptr<IBatchAction> act = std::make_unique<ActionClass>(new TestTxn());
    initialize_txn_to_random_values(act);
    actions.push_back(std::move(act));
  }

  return actions;
};

template <class ActionClass> 
void ActionFactory<ActionClass>::initialize_txn_to_random_values(
    IBatchAction* act) {
  fill_disjoint_set_of_random_numbers(
      get_nonzero_integer(read_num_distro),
      read_distro,
      read_set);

  fill_disjoint_set_of_random_numbers(
      get_nonzero_integer(write_num_distro),
      write_distro,
      write_set,
      read_set);

  // set the appropriate values
  for (auto& key : read_set) act->add_read_key(key);
  for (auto& key : write_set) act->add_write_key(key);
}; 

template <class ActionClass> 
void ActionFactory<ActionClass>::initialize_txn_to_random_values(
    std::unique_ptr<IBatchAction>& act) {
  initialize_txn_to_random_values(act.get());
};

template <class ActionClass>
void ActionFactory<ActionClass>::set_action_spec(ActionSpecification spec) {
  initialize_from_spec(spec);   
};

#endif // BATCH_ACTION_FACTORY_IMPL_
