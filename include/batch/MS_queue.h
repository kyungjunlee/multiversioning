#ifndef _MS_QUEUE_H_
#define _MS_QUEUE_H_

// TODO:
//    - Memory Management. Right now we have memory leakage within the destructor
//    since the lock stages are never deallocated!
//    - Overloading of new/delete.

/*
 *    MS Queue
 *
 *    A latch-free, one-consumer, one-producer queue of LockStages. Resembles
 *    SimpleQueue, but does not reserve memory up-front.
 *
 *    Since this is the queue that is present within the lock table of 
 *    global schedule, we only ever pop head and merge a batch queue in.
 *
 *    The queue implementation is a modified MS-queue implementation that is
 *    lock-free and non-blocking. See https://www.research.ibm.com/people/m/michael/podc-1996.pdf.
 */

template <typename Elt>
class MSQueue {
public:
  MSQueue();

  // Queue Element class definition
  class QueueElt {
  public:
    QueueElt();
    QueueElt(Elt* e);
    Elt* contents;
    QueueElt* next;
    
    Elt* get_contents();
    QueueElt* get_next_elt();
    bool set_next_elt(QueueElt* ne);
  };

  QueueElt* head;
  QueueElt* tail; 
  
  // user interface
  bool is_empty() const;
  
  QueueElt* peek_tail_elt() const;
  QueueElt* peek_head_elt() const;
  Elt* peek_tail() const;
  Elt* peek_head() const; 
  
  Elt* try_pop_head();
  
  // The implicit assumption is that lq is immutable at the time of
  // merging. 
  void merge_queue(MSQueue<Elt>* lq);
};

#include "batch/MS_queue_impl.h"

#endif // _MS_QUEUE_H_
