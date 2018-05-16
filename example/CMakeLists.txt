add_executable(raft_demo demo.cc)
target_link_libraries(raft_demo raft)
install(TARGETS raft_demo DESTINATION bin)