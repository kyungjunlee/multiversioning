#include "batch/executor.h"

#include <utility>

Executor::Executor(ExecutorConfig cfg):
    Runnable(cfg.m_cpu_number) {
  input_queue = std::move(cfg.input_queue);
  output_queue = std::move(cfg.output_queue);
  pending_queue = std::make_unique<PendingQueue>();
  // global_schedule = cfg.global_schedule;
};

void Executor::StartWorking() {
  // TODO: introduce a "kill" flag.
  while (true) {
    // get a batch to execute or busy wait until we may do that
    while ((currentBatch = input_queue.try_pop_head()) != nullptr);

    ProcessActionBatch();
  }
};

void Executor::Init() {

};

void Executor::enqueue_batch(std::vector<BatchAction>&& batch) {
  input_queue.push_tail(std::forward(batch));
};

void Executor::process_action_batch() {
  for (unsigned int i = 0; i < currentBatch->size(); i++) {
    // attempt to execute the pending actions first. This is because the 
    // actions "earlier" in the current batch are more likely to not
    // be blocked by other actions!
    process_pending();

    if(!process_action(currentBatch->at(i)) {
      pending_queue->push_back(&(currentBatch->at(i)));        
    } 
  } 

  // make sure that everything within current batch has been successfully executed!
  while (!pending_queue->empty()) {
    process_pending();
  }

  // put the batch to the output queue!
  output_queue->push_tail(std::move(*currentBatch));
};

void Executor::process_action(BatchAction* act) {
  assert(act != nullptr);

  uint64_t action_state = act->action_state;
  barrier();

  // check the state of the action
  if (action_state == static_cast<uint64_t>(BatchActionState::done)) {
    // claim the action if there is no one else who already did.
    if (act->conditional_atomic_change_state(
          BatchActionState::substantiated,
          BatchActionState::processing)) {
      // we successfully claimed the action.
      if (act->ready_to_execute()) {
        // TODO: run the action.
        // TODO: finalize with the stage it belongs to through the 
        //    global schedule!
        bool state_change_success = act->confitional_atomic_change_state(
            BatchActionState::processing,
            BatchActionState::done);
        assert(state_change_success);
        return true;
      } else {
        // attempt to execute blockers.
        auto execute_blockers = [this, act](BatchAction::RecSet* set) {
          LockStage* blocking_stage = nullptr;
          const LockStage::RequestingActions blocking_actions;
          for (auto rec_key : *set) {
            // TODO:
            // blocking_stage = global_schedule->get_head_for_record 
            // or get_first_stage_for_record().
            blocking_actions = blocking_stage->get_requesters();
            if (blocking_actions.find(act) != blocking_actions.end()) {
              // this is not a blocking stage. act is clearly at the head
              // of this lock queue!
              continue;
            }

            for (auto action_sptr : blockers) {
              this->process_action(action_ptr.get()); 
            }
          } 
        };
        
        execute_blockers(act->get_writeset_handle());
        execute_blockers(act->get_readset_handle());
        
        // unlock the action so that someone else may attempt to execute it.
        bool state_change_success = act->confitional_atomic_change_state(
            BatchActionState::processing,
            BatchActionState::substantiated);
        assert(state_change_success);
        return false;
      }
     
//      if (action_ready(act)) {
//        // run the action run function here
//        // TODO
//        return true;
//      } else {
//        // reset the state
//        bool state_change_success;
//        state_change_success = act->conditional_atomic_change_state(
//            BatchActionState::processing,
//            BatchActionState::substantiated);
//        assert(state_change_success);
//
//        return false;
//      }
    }
  }

  // the action is in done state! Nothing to do.
  return true;
};

void Executor::process_pending() {
  // attempt to execute everything within the pending queue
  auto it = pending_list->begin();
  while (it != pending_list->end()) {
    if (process_action(*it)) {
      it = pending_list.erase(it);
      continue;
    }

    it ++;
  }
}
