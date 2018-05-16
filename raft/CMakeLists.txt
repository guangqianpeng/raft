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
        COMMAND ${CMAKE_STUB_FORMATTER}
        ARGS -i ${stub_dir}/RaftServiceStub.h ${stub_dir}/RaftClientStub.h
        DEPENDS raw_header
        COMMENT "clang format Stub..."
        VERBATIM
)

add_library(raft STATIC
        Raft.cc Raft.h
        RaftPeer.cc RaftPeer.h
        RaftService.h RaftService.cc
        RaftServiceStub.h RaftClientStub.h
        Log.h Log.cc
        Storage.cc Storage.h
        Node.cc Node.h
        Config.h Struct.h Random.h Callback.h)
target_link_libraries(raft jrpc leveldb)
install(TARGETS raft DESTINATION lib)