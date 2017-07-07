#ifndef _MEMORY_POOL_IMPL_H_
#define _MEMORY_POOL_IMPL_H_

template <class AllocatedClass>
void MemoryPool<AllocatedClass>::allocate_new_element() {
  free_list.push_tail(new MemEltQueue::MemElt(sizeof(AllocatedClass))); 
};

template <class AllocatedClass>
void* MemoryPool<AllocatedClass>::alloc() {
  if (free_list.is_empty()) {
    allocate_new_element();
  }

  auto mem_elt = free_list.pop_head();
  auto allocation_memory = mem_elt->memory;
  mem_elt->memory = nullptr;
  allocated_list.push_tail(mem_elt);

  return (void*) allocation_memory;
};

template <class AllocatedClass>
void MemoryPool<AllocatedClass>::free(void* elt) {
  assert (allocated_list.is_empty() == false);  
  auto mem_elt = allocated_list.pop_head();
  mem_elt->memory = (char*) elt;
  
  free_list.push_tail(mem_elt);
};

template <class AllocatedClass>
void MemoryPool<AllocatedClass>::preallocate_memory(unsigned int num) {
  for (unsigned int i = 0; i < num; i++) {
    allocate_new_element();
  }
};

template <class AllocatedClass>
unsigned int MemoryPool<AllocatedClass>::available_elts() {
  return free_list.size();
};

template <class AllocatedClass>
MemoryPool<AllocatedClass>::~MemoryPool() {
  assert(allocated_list.is_empty() == true);
};

#endif //_MEMORY_POOL_IMPL_H_
