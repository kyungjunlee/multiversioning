#include "batch/executor.h"

#include <utility>

BatchExecutor::BatchExecutor(
      ExecutorThreadManager* manager, 
      int m_cpu_number):
    ExecutorThread(manager, m_cpu_number) {
  this->input_queue = std::make_unique<ExecutorQueue>();;
  this->output_queue = std::make_unique<ExecutorQueue>();
  this->pending_list = std::make_unique<PendingList>();
};

void BatchExecutor::StartWorking() {
  while (!is_stop_requested()) {
    // get a batch to execute or busy wait until we may do that
    currentBatch.reset(nullptr);

    while(input_queue->is_empty()) {
      if (is_stop_requested()) return; 
    }
    currentBatch = std::move(input_queue->peek_head());
    input_queue->pop_head();

    process_action_batch();
  }
};

void BatchExecutor::Init() {
};

void BatchExecutor::add_actions(ExecutorThread::BatchActions&& actions) {
  input_queue->push_tail(
      std::move(
        std::make_unique<ExecutorThread::BatchActions>(
          std::move(actions))));
};

void BatchExecutor::process_action_batch() {
  for (unsigned int i = 0; i < currentBatch->size(); i++) {
    // attempt to execute the pending actions first. This is because the 
    // actions "earlier" in the current batch are more likely to not
    // be blocked by other actions!
    process_pending();

    assert(currentBatch->at(i) != nullptr);
    if(!process_action(currentBatch->at(i))) {
      pending_list->push_back(currentBatch->at(i));        
      pending_list->size();
    } 
  } 

  // make sure that everything within current batch has been successfully executed!
  while (!pending_list->empty()) {
    process_pending();
  }

  // put the batch to the output queue!
  output_queue->push_tail(std::move(currentBatch));
};

bool BatchExecutor::process_action(std::shared_ptr<IBatchAction> act) {
  assert(act != nullptr);

  uint64_t action_state = act->action_state;
  barrier();

  if (action_state == static_cast<uint64_t>(BatchActionState::done)) {
    // the action is in done state! Nothing to do.
    return true;
  }

  // attempt to claim the action
  if (!act->conditional_atomic_change_state(
        BatchActionState::substantiated,
        BatchActionState::processing)) {
    // could not claim the action -- someone else must be processing it.
    return false;
  }

  // we successfully claimed the action.
  if (act->ready_to_execute()) {
    act->Run(this->exec_manager->get_db_storage_pointer());
    this->exec_manager->finalize_action(act); 
    bool state_change_success = act->conditional_atomic_change_state(
        BatchActionState::processing,
        BatchActionState::done);
    assert(state_change_success);
    return true;
  } else {
    // attempt to execute blockers.
    auto execute_blockers = [this, act](
        IBatchAction::RecordKeySet* set) {
      std::shared_ptr<LockStage> blocking_stage = nullptr;
      for (auto rec_key : *set) {
        blocking_stage = 
          this->exec_manager->get_current_lock_holder_for(rec_key);
        const LockStage::RequestingActions& blocking_actions = 
          blocking_stage->get_requesters();

        if (blocking_actions.find(act) != blocking_actions.end()) {
          // this is not a blocking stage. act is clearly at the head
          // of this lock queue!
          continue;
        }

        for (auto action_sptr : blocking_actions) {
          this->process_action(action_sptr); 
        }
      } 
    };
    
    execute_blockers(act->get_writeset_handle());
    execute_blockers(act->get_readset_handle());
    
    // unlock the action so that someone else may attempt to execute it.
    bool state_change_success = act->conditional_atomic_change_state(
        BatchActionState::processing,
        BatchActionState::substantiated);
    assert(state_change_success);
    return false;
  }
};

void BatchExecutor::process_pending() {
  // attempt to execute everything within the pending queue
  auto it = pending_list->begin();
  while (it != pending_list->end()) {
    if (process_action(*it)) {
      it = pending_list->erase(it);
      continue;
    }

    it ++;
  }
};

std::unique_ptr<ExecutorThread::BatchActions> BatchExecutor::try_get_done_batch() {
  if (!output_queue->is_empty()) {
    std::unique_ptr<ExecutorThread::BatchActions> act;
    act = std::move(output_queue->peek_head());
    output_queue->pop_head();
    return act; 
  }

  return nullptr;
};

void BatchExecutor::signal_stop_working() {
  xchgq(&stop_signal, 1);
}

bool BatchExecutor::is_stop_requested() {
  return stop_signal;
}
