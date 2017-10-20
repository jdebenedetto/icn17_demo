if(NOT COMPILE_FOR_IOS)
  add_executable(metis_anp_daemon metis_anp_daemon.c)
  target_link_libraries(metis_anp_daemon ${METIS_LINK_LIBRARIES})

  install(TARGETS metis_anp_daemon RUNTIME DESTINATION bin)
endif()
