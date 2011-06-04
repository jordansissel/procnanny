#include "insist.h"
#define BAR "Hello"

int main() {
  int i = 4;
  printf("foo" BAR  "\n");
  insist_return(i == 3, 5, "Something went wrong, wanted i == 3, got %d", i);
  return 0;
}
