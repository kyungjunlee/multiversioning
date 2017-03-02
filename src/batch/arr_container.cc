#include <batch/arr_container.h>
#include <algorithm>

bool ArrayContainer::arr_is_empty() {
  return current_min_index >= this->actions_uptr->size() ||
    current_min_index < current_barrier_index;
}

BatchAction* ArrayContainer::peek_curr_elt() {
  if (arr_is_empty()) return nullptr;

  // return the pointer within unique_ptr of the right elt.
  return ((*this->actions_uptr)[current_min_index].get());
}

ArrayContainer::action_uptr ArrayContainer::take_curr_elt() {
  if (arr_is_empty()) return nullptr;

  // swap the current min with the current barrier index one. That
  // puts the element into the "removed elements". Note that 
  // this does not free memory etc.
  action_uptr min = std::move((*this->actions_uptr)[current_min_index]);
  (*this->actions_uptr)[current_min_index] = 
    std::move((*this->actions_uptr)[current_barrier_index]);

  advance_to_next_elt();
  current_barrier_index ++;
  return min;
}

void ArrayContainer::advance_to_next_elt() {
  current_min_index ++;
}

void ArrayContainer::sort_remaining() {
  std::sort(
      this->actions_uptr->begin() + current_barrier_index,
      this->actions_uptr->end(),
      // NOTE: this makes use of an overloaded < operator for Actions!
      [](action_uptr const& a, action_uptr const& b) {return *a < *b;});

  current_min_index = current_barrier_index;
}

uint32_t ArrayContainer::get_remaining_count() {
  return this->actions_uptr->size() - current_barrier_index;
}
