#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#include <batch/batch_action_interface.h>

#include <memory>
#include <vector>
#include <stdint.h>

// Fully abstract class specifying the behavior of a container. 
//
// A container is a class that keeps track of actions within a batch and
// orders them according to a function of the number of exclusive and 
// shared locks requested.
//
// The container allows us to obtain the elements in an increasing order
// skipping some of them.
class Container {
public:
  typedef std::vector<std::unique_ptr<IBatchAction>> BatchActions;
protected:
  std::unique_ptr<BatchActions> actions_uptr;

  Container() = delete;
  Container(const Container& c) = delete;
  Container(std::unique_ptr<BatchActions> actions):
    actions_uptr(std::move(actions))
  {};

public:
  /*
   * Get an observing pointer to the current element.
   *
   * Returns nullptr if no elements are left.
   */
  virtual IBatchAction* peek_curr_elt() = 0;
  /*
   * Obtain ownership over the currently minimum element. The element
   * is removed from the container. curr_elt is advanced to the next element.
   *
   * Returns nullptr if no elements are left.
   */
  virtual std::unique_ptr<IBatchAction> take_curr_elt() = 0;
  /*
   * Get the number of elements remaining in the container.
   */
  virtual uint32_t get_remaining_count() = 0;
  /*
   * Advance to the next minimum element.
   */
  virtual void advance_to_next_elt() = 0;
  /*
   * Sort the elements remaining in the container.
   *
   * Resets curr_elt to the global minimum.
   */
  virtual void sort_remaining() = 0;
 
  virtual ~Container(){};
};

#endif //_CONTAINER_H_
