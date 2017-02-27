#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#include "mv_action.h"

#include <memory>
#include <vector>

// Fully abstract class specifying the behavior of a container. 
//
// A container is a class that keeps track of actions within a batch and
// orders them according to a function of the number of exclusive and 
// shared locks requested.
template <class Action>
class Container {
  typedef std::unique_ptr<Action> action_uptr;
  typedef std::vector<action_uptr> actions_vec;
protected:
  std::unique_ptr<actions_vec> actions_uptr;

  Container() = delete;
  Container(const Container& c) = delete;
  Container(std::unique_ptr<actions_vec> actions):
    actions_uptr(std::move(actions))
  {};

  /*
   * Advance to the next minimum element.
   */
  virtual void advance_to_next_min() = 0;
  /*
   * Sort the elements remaining in the container.
   */
  virtual void sort_remaining() = 0;
 
public:
  /*
   * Get an observing pointer to the currently minimal element.
   * Returns nullptr if no elements are left.
   */
  virtual Action* peak_curr_min_elt() = 0;
  /*
   * Obtain ownership over the currently minimum element. The element
   * is removed from the container.
   * Returns nullptr if no elements are left.
   */
  virtual action_uptr take_curr_min_elt() = 0;
  /*
   * Get the number of elements remaining in the container.
   */
  virtual unsigned int get_remaining_count() = 0;

  virtual ~Container(){};
}
