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

struct MemElt {
  struct MemElt* next_elt;
  char* memory;

  MemElt(unsigned int byte_size) {
    next_elt = nullptr;
    memory = new char[byte_size];
  };

  ~MemElt() {
    next_elt = nullptr;
    delete[] memory;
  };
};

struct MemEltList {
  MemElt* head;
  MemElt* tail;

  MemEltList() {
    head = nullptr;
    tail = nullptr;
  };

  void push_tail(MemElt* me) {
    if (is_empty()) {
      head = me;
      tail = head;
    } else {
      tail->next_elt = me;
      tail = me;
    }
  };

  MemElt* pop_head() {
    if (is_empty()) {
      return nullptr;
    }

    auto tmp = head;
    head = head->next_elt;
    if (head == nullptr) {
      assert(tail == tmp);
      tail = nullptr;
    }

    return tmp;
  };

  bool is_empty() {
    return (head == nullptr);
  };

  ~MemEltList() {
    MemElt* tmp;
    while (!is_empty()) {
      tmp = head;
      head = head->next_elt;

      delete tmp;
    }
  }
};

template <class ActionClass>
class BatchActionAllocator {
private:
  //  TODO: 
  // The above queue would need to be SPSC. Considering using 
  // MR queue if possible. I think it should be possible here, but the 
  // memory allocated in MR queue must also be taken care of by a pool
  // allocator somewhere, lol.
  //
  // That allocator can be local to the queue though so that shouldn't be an issue.
  // Ever. Just need to modify the MRqueue a tad.
  MemEltList act_free_list;
  MemEltList act_allocated_list;
  MemEltList txn_free_list;
  MemEltList txn_allocated_list;

  MemElt* act_cur_elt;
  MemElt* txn_cur_elt;
  unsigned int batch_size;
  unsigned int next_mem_elt;

  void allocate_more_memory() {
    act_free_list.push_tail(new MemElt(sizeof(ActionClass) * batch_size));
    txn_free_list.push_tail(new MemElt(sizeof(TestTxn) * batch_size));
  }
public:
  BatchActionAllocator(unsigned int batch_size): 
    act_cur_elt(nullptr),
    txn_cur_elt(nullptr),
    batch_size(batch_size),
    next_mem_elt(batch_size)
  {};

  // In generality we must provide memory for txns or provide an array 
  // of such txns. This should be done using Args&&... args and variadic templates.
  ActionClass* get_action() {
    if (next_mem_elt == batch_size) {
      if (act_free_list.is_empty() || txn_free_list.is_empty()) {
        allocate_more_memory();
      }
      assert(act_free_list.is_empty() == false);
      assert(txn_free_list.is_empty() == false);

      // save the element to allocated list.
      act_allocated_list.push_tail(act_cur_elt);
      txn_allocated_list.push_tail(txn_cur_elt);

      // get the new memory element.
      act_cur_elt = act_free_list.pop_head();
      txn_cur_elt = txn_free_list.pop_head();
      next_mem_elt = 0;
    }

    ActionClass* act_mem = (ActionClass*)
      &act_cur_elt->memory[next_mem_elt * sizeof(ActionClass)];
    TestTxn* txn_mem = (TestTxn*) 
      &txn_cur_elt->memory[next_mem_elt * sizeof(TestTxn)];
    next_mem_elt ++;

    return new (act_mem) ActionClass(new (txn_mem) TestTxn());
  };

  void free_last_action_batch() {
    act_free_list.push_tail(act_allocated_list.pop_head());
    txn_free_list.push_tail(txn_allocated_list.pop_head());
  };

  // TODO:
  //  Add assertions about the number of elements being on free list and 
  //  allocated list. We probably would expect ALL of the elements ever allocated
  //  to be on the free list, not allocated list. Plus, we probably want the number
  //  of such elements to be checked as well. Right?
  ~BatchActionAllocator () {
    delete act_cur_elt;
    delete txn_cur_elt;
  };
};

#endif // BATCH_ACTION_FACTORY_H_
