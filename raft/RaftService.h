//
// Created by frank on 18-4-19.
//

#ifndef RAFT_RAFTSERVICE_H
#define RAFT_RAFTSERVICE_H

#include <tinyev/EventLoop.h>

#include <raft/RaftServiceStub.h>
#include <raft/Callback.h>

class RaftService: public jrpc::RaftServiceStub<RaftService>
{
public:
    explicit
    RaftService(jrpc::RpcServer& server);

    void SetDoRequestVoteCallback(const raft::DoRequestVoteCallback& cb)
    { doRequestVote_ = cb; }

    void SetDoAppendEntriesCallback(const raft::DoAppendEntriesCallback& cb)
    { doAppendEntries_ = cb; }

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
    raft::DoRequestVoteCallback doRequestVote_;
    raft::DoAppendEntriesCallback doAppendEntries_;
};


#endif //RAFT_RAFTSERVICE_H
