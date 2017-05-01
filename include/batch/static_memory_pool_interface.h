#ifndef _STATIC_MEMORY_POOL_INTERFACE
#define _STATIC_MEMORY_POOL_INTERFACE

class IStaticMemoryPool {
private:
  unsigned int init_size;
public:
  IStaticMemoryPool(unsigned int init_size): init_size(init_size) {};
  virtual void* allocate(size_t size) = 0;
  virtual void free(void* toDelete) = 0;
  virtual ~IStaticMemoryPool ();
};

#endif // _STATIC_MEMORY_POOL_INTERFACE
