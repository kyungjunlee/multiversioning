#include <batch/arr_container.h>
#include <algorithm>

ArrayContainer::ArrayContainer(BatchActions&& actions):
    Container(std::move(actions)),
    elements_removed_total(0),
    elements_removed_this_round(0),
    current_element(0)
{
  std::sort(
      this->actions.begin(),
      this->actions.end(),
      // NOTE: this makes use of an overloaded < operator for Actions!
      [](
        IBatchAction* const& a, 
        IBatchAction* const& b) 
      {return *a < *b;});
};

bool ArrayContainer::arr_is_empty() {
  return current_element >= this->actions.size() ||
    this->actions[current_element] == nullptr;
}

IBatchAction* ArrayContainer::peek_curr_elt() {
  if (arr_is_empty()) {
    return nullptr;
  }

  // return the pointer within unique_ptr of the right elt.
  return this->actions[current_element].get();
}

IBatchAction* ArrayContainer::take_curr_elt() {
  if (arr_is_empty() || current_element == this->actions.size()) return nullptr;

  elements_removed_this_round ++;
  elements_removed_total ++;
  current_element ++;
  return std::move(this->actions[current_element - 1]);
}

void ArrayContainer::advance_to_next_elt() {
  // if something has been deleted this round, move the element to the "head"
  // of contiguous unremoved space.
  if (elements_removed_this_round > 0) {
    this->actions[current_element - elements_removed_this_round] =
      std::move(this->actions[current_element]);
  }

  current_element ++;
}

void ArrayContainer::sort_remaining() {
  elements_removed_this_round = 0;
  current_element = 0;
}

uint32_t ArrayContainer::get_remaining_count() {
  return this->actions.size() - elements_removed_total;
}
