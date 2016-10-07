# Lcool Testsuite
#  Currently this runs a load of functional tests over the lcoolc binary

add_executable(test_driver driver.cpp test.hpp testcases.cpp)
add_test(NAME lcooltests COMMAND test_driver $<TARGET_FILE:lcoolc>)
