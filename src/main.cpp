#include <iostream>
#include "XScreenSize.hpp"

int main (int argc, char **argv) {
  XScreenSize::Getter sizeGetter;

  std::cout << sizeGetter.getScreen() << std::endl;
  for(auto& output : sizeGetter.getOutputs())
    std::cout << "\t" << output << std::endl;
  
  return 0;
}
