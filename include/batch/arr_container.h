#ifndef _ARR_CONTAINER_H_
#define _ARR_CONTAINER_H_

#include <batch/container.h>
// TODO: Overload the new and delete operators to use static memory allocators
//    when that is implementsed.
// TODO: This is a simple and naive implementation. Time it and decide whether we
//    require a more sophisticated approach.

/*
 *  Implementation of the container abstract class.
 *
 *  The container is basically an array with two indexes.
 *    |--|--|--|--|--|--|--|--|--|--|--|
 *                     ^-- current_min_index
 *         ^----- current_barrier_index
 *    |-----| -- removed elements
 *
 *          |--------| -- inspected, but not removed elements
 *  
 *                   |-----------------| -- not inspected, not removed elements.
 *
 *    Usage is described within include/batch/container.h
 */
class ArrayContainer : public Container {
private:
  typedef Container::BatchActions BatchActions; 
  uint32_t current_min_index;
  uint32_t current_barrier_index;
protected:
  ArrayContainer() = delete;
  ArrayContainer(const ArrayContainer& ac) = delete;

  bool arr_is_empty();

public:
  ArrayContainer(std::unique_ptr<BatchActions> actions);

  IBatchAction* peek_curr_elt() override;
  std::unique_ptr<IBatchAction> take_curr_elt() override;
  void advance_to_next_elt() override;
  void sort_remaining() override;
  uint32_t get_remaining_count() override;
  virtual ~ArrayContainer() {};
};

#endif // _ARR_CONTAINER_H_
