#ifndef DB_TEST_HELPER_H_
#define DB_TEST_HELPER_H_

#include "batch/supervisor_interface.h"
#include "batch/batch_action_interface.h"
#include "batch/batch_action.h"
#include "test/test_action.h"
#include "batch/txn_factory.h"

#include <cassert>
#include <memory>
#include <vector>
#include <random>

template <class SupervisorInterfaceChild>
class DBTestHelper {
private:
  DBStorageConfig db_conf;
  SchedulingSystemConfig sched_conf;
  ExecutingSystemConfig exec_conf;
  unsigned int action_num;
  std::vector<std::unique_ptr<IBatchAction>> workload;

  bool test_properly_initialized() {
    return 
      db_conf.tables_definitions.size() > 0 &&
      sched_conf.scheduling_threads_count > 0 &&
      sched_conf.batch_size_act > 0 &&
      exec_conf.executing_threads_count > 0 &&
      (action_num > 0 || workload.size() > 0);
  }
public:
  typedef std::function<void (IDBStorage* db)> DBAssertion;

  DBTestHelper():
    db_conf({{}}),
    sched_conf({0,0,0,0}),
    exec_conf({0, 0}),
    action_num(0)
  {};

  DBTestHelper& set_table_info(unsigned int tables, unsigned int recs_per_table) {
    for (unsigned int i = 0; i < tables; i++) {
      db_conf.tables_definitions.push_back({i, recs_per_table});
    }  
    return *this;
  };
  DBTestHelper& set_exec_thread_num(unsigned int threads) {
    exec_conf.executing_threads_count = threads; 
    return *this;
  };
  DBTestHelper& set_sched_thread_num(unsigned int threads) {
    sched_conf.scheduling_threads_count = threads;
    return *this;
  };
  DBTestHelper& set_batch_size(unsigned int size) {
    sched_conf.batch_size_act = size;
    return *this;
  };
  DBTestHelper& set_batch_timeout(unsigned int sec) {
    sched_conf.batch_length_sec = sec;
    return *this;
  };
  DBTestHelper& set_action_num(unsigned int acts) {
    action_num = acts; 
    return *this;
  }
  DBTestHelper& set_workload(
      std::vector<std::unique_ptr<IBatchAction>>&& work) {
    workload = std::move(work);
    return *this;
  }

  void runTest(DBAssertion assertion = [](IDBStorage* db){(void) db;}) {
    assert(test_properly_initialized());
    sched_conf.first_pin_cpu_id = 1;
    exec_conf.first_pin_cpu_id = sched_conf.scheduling_threads_count + 1;

    std::unique_ptr<ISupervisor> s = std::make_unique<SupervisorInterfaceChild>(
       db_conf, sched_conf, exec_conf);

    if (workload.size() == 0) {
      assert(action_num > 0);
      assert(db_conf.tables_definitions.size() > 0);

      unsigned int recs_in_table = db_conf.tables_definitions[0].num_records;
      LockDistributionConfig lock_distro = {
        .low_record = 0,
        .high_record = recs_in_table - 1,
        .average_num_locks = 5,
        .std_dev_of_num_locks = 0
      }; 
      ActionSpecification act_spec = {
        .writes = lock_distro, 
        .reads = lock_distro
      };

      workload = ActionFactory<BatchAction>::generate_actions(
          act_spec, action_num);
    } else {
      action_num = workload.size();
    }
    s->set_simulation_workload(std::move(workload));

    barrier();

    s->init_system();
    s->start_system();

    barrier();

    std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> outputs;
    unsigned int output_count = 0;
    while (output_count != action_num) {
      auto o = s->get_output();
      if (o == nullptr) continue;

      output_count += o->size();
      outputs.push_back(std::move(o));
    }

    barrier();

    assertion(s->get_db_pter());
    s->stop_system();
  }
};

#endif // DB_TEST_HELPER_H_
