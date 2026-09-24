if (HAVE_TESTS)
  add_executable(workrave-libs-commonui-sitstand-test SitStandTimerTest.cc)
  target_code_coverage(workrave-libs-commonui-sitstand-test AUTO)

  target_link_libraries(workrave-libs-commonui-sitstand-test PRIVATE workrave-libs-commonui)
  target_link_libraries(workrave-libs-commonui-sitstand-test PRIVATE GTest::gtest_main)
  target_link_libraries(workrave-libs-commonui-sitstand-test PRIVATE ${EXTRA_LIBRARIES})

  if (SSP_LIBRARY)
    target_link_libraries(workrave-libs-commonui-sitstand-test PRIVATE ${SSP_LIBRARY})
  endif()

  workrave_add_test(workrave-libs-commonui-sitstand-test)
endif()
