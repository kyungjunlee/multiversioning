#include "batch/packing.h"
#include "batch/batch_action.h"

bool Packer::txn_conflicts(
    BatchAction* t,
    RecordSet* ex_locks_in_packing, 
    RecordSet* sh_locks_in_packing) {
  auto t_ex = t->get_writeset_handle();
  auto t_sh = t->get_readset_handle();

  // We will be iterating over sets. Always pick the smaller one to
  // iterate over.
  RecordSet* smaller;
  RecordSet* larger;

  auto conflictExists = 
       [&smaller, &larger](RecordSet* rs1, RecordSet* rs2){

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

Packer::ActionUptVector Packer::get_packing(Container* c) {
  RecordSet held_ex_locks;
  RecordSet held_sh_locks; 

  ActionUptVector action_in_packing;
  BatchAction* next_action;
  Action_upt action;

  auto merge_sets = [](RecordSet* mergeTo, RecordSet* mergeFrom) {
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
      action_in_packing.push_back(std::move(action));
      continue;
    }
    c->advance_to_next_elt();
  }

  return action_in_packing;
}
