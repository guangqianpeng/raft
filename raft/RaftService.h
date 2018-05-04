//
// Created by frank on 18-4-19.
//

#ifndef RAFT_RAFTSERVICE_H
#define RAFT_RAFTSERVICE_H

#include <raft/Raft.h>
#include <raft/RaftServiceStub.h>

class RaftService: public jrpc::RaftServiceStub<RaftService>
{
public:
    RaftService(jrpc::RpcServer& server, Raft& raft);

    void AddRaftPeer(const ev::InetAddress& serverAddress);

    void StartRaft();

    void RequestVote(int term,
                     int candidateId,
                     int lastLogIndex,
                     int lastLogTerm,
                     const jrpc::UserDoneCallback& done);
    void AppendEntries(int term,
                       int prevLogIndex,
                       int prevLogTerm,
                       json::Value entries,
                       int leaderCommit,
                       const jrpc::UserDoneCallback& done);

private:
    Raft& raft_;
};


#endif //RAFT_RAFTSERVICE_H
