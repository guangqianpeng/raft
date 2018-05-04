add_custom_command(
        OUTPUT raw_header
        COMMAND jrpcstub
        ARGS -o -i ${CMAKE_CURRENT_SOURCE_DIR}/spec.json
        MAIN_DEPENDENCY spec.json
        DEPENDS jrpcstub
        COMMENT "Generating Server/Client Stub..."
        VERBATIM
)

set(stub_dir ${PROJECT_BINARY_DIR}/raft)

add_custom_command(
        OUTPUT RaftServiceStub.h RaftClientStub.h
        COMMAND clang-format
        ARGS -i ${stub_dir}/RaftServiceStub.h ${stub_dir}/RaftClientStub.h
        DEPENDS raw_header
        COMMENT "clang format Stub..."
        VERBATIM
)

add_executable(raft
        Raft.cc Raft.h
        RaftPeer.cc RaftPeer.h
        RaftService.h RaftService.cc
        RaftServiceStub.h RaftClientStub.h
        main.cc Random.h Callback.h LogSlice.h)
target_link_libraries(raft jrpc)