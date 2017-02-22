#ifndef _TEST_MS_QUEUE_H_
#define _TEST_MS_QUEUE_H_

#include "batch/MS_queue.h"

template <typename Elt>
class TestMSQueue : public MSQueue<Elt> {
  public:
    void non_concurrent_push_tail(Elt* e) {
      this->tail->set_next_elt(new typename MSQueue<Elt>::QueueElt(e));
      this->tail = this->tail->get_next_elt();
    }
};

#endif
