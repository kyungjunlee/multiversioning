#include "batch/packing.h"
#include "batch/batch_action_interface.h"

bool Packer::txn_conflicts(
    IBatchAction* t,
    RecordKeySet* ex_locks_in_packing, 
    RecordKeySet* sh_locks_in_packing) {
  auto t_ex = t->get_writeset_handle();
  auto t_sh = t->get_readset_handle();

  // We will be iterating over sets. Always pick the smaller one to
  // iterate over.
  RecordKeySet* smaller;
  RecordKeySet* larger;

  auto conflictExists = 
       [&smaller, &larger](RecordKeySet* rs1, RecordKeySet* rs2){

    // pick the smaller set to iterate over.
    if (rs1->size() > rs2->size()) {
      smaller = rs1;
      larger = rs2;
    } else {
      smaller = rs2;
      larger = rs1;
    }

    for (auto it = smaller->begin(); it != smaller->end(); it++) {
      // return true if the larger set contains any of the elements of 
      // the smaller set.
      if (larger->find(*it) != larger->end()) {
        return true;
      }
    }

    return false;
  };
  
  // conflict exists between 
  //   1) the set of exclusive locks requested and both exclusive and shared
  //    locks already requested within the batch
  //   2) the set of shared locks requested and the set of exlusive locks requested
  //    already.
  return (conflictExists(t_ex, ex_locks_in_packing) ||
      conflictExists(t_ex, sh_locks_in_packing) ||
      conflictExists(t_sh, ex_locks_in_packing));
}

Packer::BatchActions Packer::get_packing(Container* c) {
  RecordKeySet held_ex_locks;
  RecordKeySet held_sh_locks; 

  BatchActions actions_in_packing;
  IBatchAction* next_action;
  std::unique_ptr<IBatchAction> action;

  auto merge_sets = [](RecordKeySet* mergeTo, RecordKeySet* mergeFrom) {
    for (auto it = mergeFrom->begin(); it != mergeFrom->end(); it++) {
      mergeTo->insert(*it);
    }
  }; 

  while ((next_action = c->peek_curr_elt()) != nullptr) {
    if (!txn_conflicts(next_action, &held_ex_locks, &held_sh_locks)) {
      // add the txn to packing
      merge_sets(&held_ex_locks, next_action->get_writeset_handle());
      merge_sets(&held_sh_locks, next_action->get_readset_handle());

      // transition the ownership
      action = c->take_curr_elt();
      actions_in_packing.push_back(std::move(action));
      continue;
    }
    c->advance_to_next_elt();
  }

  return actions_in_packing;
}
