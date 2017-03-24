#ifndef BATCH_ACTION_H_
#define BATCH_ACTION_H_

#include "batch/batch_action_interface.h"

// BatchAction
//
//    Concrete implementation of actions used within our system.
class BatchAction : public IBatchAction {
  private:
    RecordKeySet readset;
    RecordKeySet writeset;
  public:
    BatchAction(txn* t): IBatchAction(t) {};

    // override the translator functions
    virtual void *write_ref(uint64_t key, uint32_t table) override;
    virtual void *read(uint64_t key, uint32_t table) override;
 
    // TODO:
    //    Tests for there
    // override the IBatchAction functions
    virtual uint64_t notify_lock_obtained() override;
    virtual bool ready_to_execute() override;

    virtual void add_read_key(RecordKey rk) override;
    virtual void add_write_key(RecordKey rk) override;
   
    virtual uint64_t get_readset_size() const override;
    virtual uint64_t get_writeset_size() const override;
    virtual RecordKeySet* get_readset_handle() override;
    virtual RecordKeySet* get_writeset_handle() override;

    // TODO: 
    //    Tests for these!
    virtual bool conditional_atomic_change_state(
        BatchActionState expected_state,
        BatchActionState new_state) override;
    virtual BatchActionState atomic_change_state(
        BatchActionState new_state) override;
    
    uint64_t action_state;

    // TODO: 
    //    Do this after we fill in the interface
    virtual void Run() override;

    virtual bool operator<(const IBatchAction& ba2) const override;
    virtual int rand() override;
};

#endif //BATCH_ACTION_H_
