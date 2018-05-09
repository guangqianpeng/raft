//
// Created by frank on 18-4-19.
//

#include <raft/Raft.h>
#include <raft/RaftPeer.h>

using namespace jrpc;

RaftPeer::RaftPeer(Raft* raft, int peer, const ev::InetAddress& serverAddress)
        : raft_(raft)
        , peer_(peer)
        , loop_(raft->GetEventLoop())
        , connected_(false)
        , serverAddress_(serverAddress)
        , rpcClient(new RaftClientStub(loop_, serverAddress))
{
    SetConnectionCallback();
}

RaftPeer::~RaftPeer() = default;

void RaftPeer::Start()
{
    AssertInLoop();
    rpcClient->start();
}

void RaftPeer::SetConnectionCallback()
{
    rpcClient->setConnectionCallback(
            [this](const ev::TcpConnectionPtr& conn) {
                bool connected = conn->connected();
                loop_->runInLoop([=](){
                    OnConnection(connected);
                });
            });
}

void RaftPeer::OnConnection(bool connected)
{
    AssertInLoop();

    connected_ = connected;
    if (!connected_) {
        rpcClient.reset(new RaftClientStub(loop_, serverAddress_));
        SetConnectionCallback();
        rpcClient->start();
    }
}


void RaftPeer::RequestVote(const RequestVoteArgs& args)
{
    AssertInLoop();

    if (!connected_)
        return;

    auto cb = [=](json::Value response, bool isError, bool timeout){
        if (isError || timeout)
            return;

        int term = response["term"].getInt32();
        bool voteGranted = response["voteGranted"].getBool();

        RequestVoteReply reply;
        reply.term = term;
        reply.voteGranted = voteGranted;
        raft_->OnRequestVoteReply(peer_, args, reply);
    };

    rpcClient->RequestVote(args.term,
                           args.candidateId,
                           args.lastLogIndex,
                           args.lastLogTerm,
                           std::move(cb));
}

void RaftPeer::AppendEntries(const AppendEntriesArgs& args)
{
    AssertInLoop();

    if (!connected_)
        return;

    auto cb = [=](json::Value response, bool isError, bool timeout) {
        if (isError || timeout)
            return;

        int term = response["term"].getInt32();
        bool success = response["success"].getBool();
        int expectIndex = response["expectIndex"].getInt32();
        int expectTerm = response["expectTerm"].getInt32();

        loop_->runInLoop([=](){
            AppendEntriesReply reply;
            reply.term = term;
            reply.success = success;
            reply.expectIndex = expectIndex;
            reply.expectTerm = expectTerm;
            raft_->OnAppendEntriesReply(peer_, args, reply);
        });
    };

    rpcClient->AppendEntries(args.term,
                             args.prevLogIndex,
                             args.prevLogTerm,
                             args.entries,
                             args.leaderCommit,
                             std::move(cb));
}
