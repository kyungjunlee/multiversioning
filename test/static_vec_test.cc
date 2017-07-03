#include <gtest/gtest.h>
#include <batch/stat_vec.h>

class StaticVectorTest : public ::testing::Test {
protected:
  const static unsigned int size = 150;
  StaticVector<int, size> vec;

  void insert_elts() {
     for (unsigned int i = 0; i < size; i++) {
      vec.insert(size - i - 1);
    }
  }
};

TEST_F(StaticVectorTest, add_ints) {
  for (unsigned int i = 0; i < size; i++) {
    ASSERT_FALSE(vec.is_full());
    ASSERT_TRUE(vec.insert(i));
  }

  ASSERT_TRUE(vec.is_full());
}

TEST_F(StaticVectorTest, first_search) {
  insert_elts();
  for (unsigned int i = 0; i < size; i++) {
    ASSERT_TRUE(vec.contains(i));
  }

  ASSERT_FALSE(vec.contains(-1));
  ASSERT_FALSE(vec.contains(size));
  ASSERT_FALSE(vec.contains(size + 1));
};

TEST_F(StaticVectorTest, sort_then_search) {
  insert_elts();
  vec.sort();

  for (unsigned int i = 0; i < size; i++) {
    ASSERT_TRUE(vec.contains(i));
  }

  ASSERT_FALSE(vec.contains(-1));
  ASSERT_FALSE(vec.contains(size));
  ASSERT_FALSE(vec.contains(size + 1));
};

TEST_F(StaticVectorTest, iteration_test) {
  ASSERT_TRUE(vec.begin() == vec.end());

  for (unsigned int i = 0; i < vec.capacity() / 2; i++) {
    vec.insert(i);
  }

  ASSERT_EQ(0, *vec.begin());
  ASSERT_EQ(vec.size()- 1, *(vec.end() - 1));

  unsigned int i = 0;
  for (auto& elt : vec) {
    ASSERT_EQ(i, elt);
    i++;
  }
}
