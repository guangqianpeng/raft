//
// Created by frank on 18-4-19.
//

#include <raft/RaftService.h>

using namespace jrpc;

RaftService::RaftService(jrpc::RpcServer& server, Raft& raft)
        : RaftServiceStub(server)
        , raft_(raft)
{}

void RaftService::AddRaftPeer(const ev::InetAddress& serverAddress)
{
    raft_.AddRaftPeer(serverAddress);
}

void RaftService::StartRaft()
{
    raft_.Start();
}

void RaftService::RequestVote(int term,
                              int candidateId,
                              int lastLogIndex,
                              int lastLogTerm,
                              const UserDoneCallback& done)
{
    RequestVoteArgs args;
    args.term = term;
    args.candidateId = candidateId;
    args.lastLogIndex = lastLogIndex;
    args.lastLogTerm = lastLogTerm;

    raft_.RequestVote(args, [=] (const RequestVoteReply& reply) {
        json::Value value(json::TYPE_OBJECT);
        value.addMember("term", reply.term);
        value.addMember("voteGranted", reply.voteGranted);
        done(std::move(value));
    });
}

void RaftService::AppendEntries(int term,
                                int prevLogIndex,
                                int prevLogTerm,
                                json::Value entries,
                                int leaderCommit,
                                const UserDoneCallback& done)
{
    AppendEntriesArgs args;
    args.term = term;
    args.prevLogIndex = prevLogIndex;
    args.prevLogTerm = prevLogTerm;
    args.entries = entries;
    args.leaderCommit = leaderCommit;

    raft_.AppendEntries(args, [=](const AppendEntriesReply& reply) {
        json::Value value(json::TYPE_OBJECT);
        value.addMember("term", reply.term);
        value.addMember("success", reply.success);
        done(std::move(value));
    });
}