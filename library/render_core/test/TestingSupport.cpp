#include "TestingSupport.hpp"

namespace triglav::test {

TestingSupport& TestingSupport::the()
{
   static TestingSupport testingSupport;
   return testingSupport;
}

}// namespace triglav::test