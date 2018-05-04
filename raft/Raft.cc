//
// Created by frank on 18-4-19.
//

#include <chrono>

#include <raft/Raft.h>
#include <raft/RaftPeer.h>

using namespace std::chrono_literals;
using std::placeholders::_1;

Raft::Raft(int me, int heartbeatTimeout, int electionTimeout)
        : me_(me)
        , heartbeatTimeout_(heartbeatTimeout)
        , electionTimeout_(electionTimeout)
        , randomGen_(me, electionTimeout, 2 * electionTimeout)
        , loop_(loopThread_.startLoop())
{
    ResetTimer();
    SetApplyCallback(std::bind(&Raft::OnApplyDefault, this, _1));
}

Raft::~Raft() = default;

void Raft::AddRaftPeer(const ev::InetAddress& serverAddress)
{
    RunTaskInLoopAndWait([&]() {
        AssertNotStarted();
        AssertInLoop();
        auto ptr = new RaftPeer(this, peerNum_, serverAddress);
        peers_.emplace_back(ptr);
        peerNum_++;
    });
}

void Raft::SetApplyCallback(const ApplyCallback& cb)
{
    RunTaskInLoopAndWait([=]() {
        AssertNotStarted();
        AssertInLoop();
        applyCallback_ = cb;
    });
}

void Raft::Start()
{
    RunTaskInLoopAndWait([=]() {
        StartInLoop();
    });
}

Raft::GetStateResult Raft::GetState()
{
    int currentTerm;
    bool isLeader;

    RunTaskInLoopAndWait([&, this]() {
        AssertStarted();

        currentTerm = currentTerm_;
        isLeader = (role_ == kLeader);
    });
    return { currentTerm, isLeader };
}

Raft::ProposeResult Raft::Propose(const json::Value& command)
{
    int index;
    int currentTerm;
    bool isLeader;

    RunTaskInLoopAndWait([&, this]() {
        AssertStarted();

        index = log_.LastLogIndex() + 1;
        currentTerm = currentTerm_;
        isLeader = (role_ == kLeader);

        if (isLeader) {
            log_.Append(currentTerm_, command);
            DEBUG("raft[%d] %s, term %d, start log %d",
                  me_, RoleString(), currentTerm_, index);
        }

        if (IsSingleNode()) {
            //
            // there is only one node in raft cluster
            // log proposed should commit and apply soon,
            // but not before Raft::Propose() return
            //
            QueueTaskInLoop([=](){
                commitIndex_ = index;
                ApplyLog();
            });
        }
    });
    return { index, currentTerm, isLeader };
};

template <typename Task>
void Raft::RunTaskInLoop(Task&& task)
{
    loop_->runInLoop(std::forward<Task>(task));
}

template <typename Task>
void Raft::QueueTaskInLoop(Task&& task)
{
    loop_->queueInLoop(std::forward<Task>(task));
}

template <typename Task>
void Raft::RunTaskInLoopAndWait(Task&& task)
{
    ev::CountDownLatch latch(1);
    RunTaskInLoop([&, this]() {
        task();
        latch.count();
    });
    latch.wait();
}

void Raft::StartInLoop()
{
    AssertInLoop();

    if (started_)
        return;
    started_ = true;

    DEBUG("raft[%d] %s, peerNum = %d starting...",
          me_, RoleString(), peerNum_);

    // output debug info every 5s
    loop_->runEvery(5s, [=](){
        DEBUG("raft[%d] %s, term %d, #votes %d, commit %d",
              me_, RoleString(), currentTerm_, votesGot_, commitIndex_);
    });

    // tick every 100ms
    loop_->runEvery(kTimeUnitInMilliseconds * 1ms,
                    [this](){ Tick(); });

    for (int i = 0; i < peerNum_; i++) {
        if (i != me_) {
            peers_[i]->Start();
        }
    }
}

void Raft::StartRequestVote()
{
    AssertInLoop();
    AssertStarted();

    RequestVoteArgs args;
    args.term = currentTerm_;
    args.candidateId = me_;
    args.lastLogIndex = log_.LastLogIndex();
    args.lastLogTerm = log_.LastLogTerm();

    for (int i = 0; i < peerNum_; i++) {
        if (i != me_) {
            peers_[i]->RequestVote(args);
        }
    }
}

void Raft::RequestVote(const RequestVoteArgs& args,
                       const RequestVoteCallback& done)
{
    RunTaskInLoop([=]() {
        RequestVoteReply reply;
        RequestVoteInLoop(args, reply);
        done(reply);
    });
}

void Raft::RequestVoteInLoop(const RequestVoteArgs& args,
                             RequestVoteReply& reply)
{
    AssertInLoop();
    AssertStarted();

    OnNewInputTerm(args.term);
    ResetTimer();

    reply.term = currentTerm_;

    if (args.term == currentTerm_ &&
        (votedFor_ == kVotedForNull || votedFor_ == args.candidateId) &&
        log_.IsUpToDate(args.lastLogIndex, args.lastLogTerm))
    {
        DEBUG("raft[%d] -> raft[%d]", me_, args.candidateId);
        votedFor_ = args.candidateId;
        reply.voteGranted = true;
    }
    else
    {
        reply.voteGranted = false;
    }
}

void Raft::OnRequestVoteReply(int peer,
                              const RequestVoteArgs& args,
                              const RequestVoteReply& reply)
{
    AssertInLoop();
    RunTaskInLoop([=]() {
        OnRequestVoteReplyInLoop(peer, args, reply);
    });
}

void Raft::OnRequestVoteReplyInLoop(int peer,
                                    const RequestVoteArgs& args,
                                    const RequestVoteReply& reply)
{
    AssertInLoop();
    AssertStarted();

    OnNewInputTerm(reply.term);

    if (role_ != kCandidate ||      // not a candidate anymore
        !reply.voteGranted ||       // vote not granted
        currentTerm_ > reply.term)  // expired vote
    {
        return;
    }

    DEBUG("raft[%d] <- raft[%d]", me_, peer);

    votesGot_++;
    if (votesGot_ > peerNum_ / 2) {
        ToLeader();
    }
}

void Raft::StartAppendEntries()
{
    AssertInLoop();
    AssertStarted();

    for (int i = 0; i < peerNum_; i++) {
        if (i == me_)
            continue;

        AppendEntriesArgs args;
        args.term = currentTerm_;
        args.prevLogIndex = nextIndex_[i] - 1;
        args.prevLogTerm = log_.TermAt(args.prevLogIndex);
        args.entries = log_.GetEntriesAsJson(nextIndex_[i], kMaxEntriesSendOneTime);
        args.leaderCommit = commitIndex_;
        peers_[i]->AppendEntries(args);
    }
}

void Raft::AppendEntries(const AppendEntriesArgs& args,
                         const AppendEntriesCallback& done)
{
    RunTaskInLoop([=]() {
        AppendEntriesReply reply;
        AppendEntriesInLoop(args, reply);
        done(reply);
    });
}

void Raft::AppendEntriesInLoop(const AppendEntriesArgs& args,
                               AppendEntriesReply& reply)
{
    AssertInLoop();
    AssertStarted();

    OnNewInputTerm(args.term);
    ResetTimer();

    reply.term = currentTerm_;

    if (currentTerm_ > args.term) {
        // expired heartbeat
        reply.success = false;
        return;
    }
    else if (role_ == kCandidate) {
        // lose leader election
        ToFollower(false);
    }
    else if (role_ == kLeader) {
        FATAL("multiple leaders in term %d", currentTerm_);
    }

    //
    // invariant here:
    //   1. role == kFollower
    //   2. args.term == currentTerm
    //
    if (log_.Contain(args.prevLogIndex, args.prevLogTerm)) {
        log_.Overwrite(args.prevLogIndex + 1, args.entries);

        //
        // update commit index monotonically
        //
        int possibleCommit = std::min(args.leaderCommit, log_.LastLogIndex());
        if (commitIndex_ < possibleCommit) {
            commitIndex_ = possibleCommit;
            ApplyLog();
        }
        reply.success = true;
    }
    else {
        reply.success = false;
    }
}

void Raft::OnAppendEntriesReply(int peer,
                                const AppendEntriesArgs& args,
                                const AppendEntriesReply& reply)
{
    AssertInLoop();
    RunTaskInLoop([=]() {
        OnAppendEntriesReplyInLoop(peer, args, reply);
    });
}

void Raft::OnAppendEntriesReplyInLoop(int peer,
                                      const AppendEntriesArgs& args,
                                      const AppendEntriesReply& reply)
{
    AssertInLoop();
    AssertStarted();

    OnNewInputTerm(reply.term);

    if (role_ != kLeader || currentTerm_ > reply.term) {
        // 1. not a leader anymore
        // 2. expired RPC(return too late)
        return;
    }

    if (!reply.success) {
        nextIndex_[peer] = std::max(matchIndex_[peer] + 1,
                                    nextIndex_[peer] - 1);
        return;
    }

    int baseIndex = args.prevLogIndex + 1;
    int entryNum = static_cast<int>(args.entries.getSize());
    int endIndex = baseIndex + entryNum;

    for (int i = baseIndex; i < endIndex; i++) {

        //
        // log[i] has already replicated on peer,
        // duplicate reply takes no effects
        //
        if (i <= matchIndex_[peer])
            continue;

        //
        // a leader cannot immediately conclude that a
        // entry from previous term is committed once it is
        // stored on majority of servers, so, just don't count #replica
        //
        if (log_.TermAt(i) != currentTerm_)
            continue;

        //
        // initial replica is 2, one for me_, one for peer
        //
        int replica = 2;
        for (int p = 0; p < peerNum_; p++) {
            if (i <= matchIndex_[p])
                replica++;
        }

        //
        // update commitIndex monotonically
        //
        if (replica > peerNum_ / 2 && commitIndex_ < i) {
            commitIndex_ = i;
        }
    }

    ApplyLog();
    if (nextIndex_[peer] < endIndex) {
        nextIndex_[peer] = endIndex;
        matchIndex_[peer] = endIndex - 1;
    }
}

void Raft::Tick()
{
    switch (role_)
    {
        case kFollower:
        case kCandidate:
            TickOnElection();
            break;
        case kLeader:
            TickOnHeartbeat();
            break;
        default:
            assert(false && "bad role");
    }
}

void Raft::OnApplyDefault(const ApplyMsg& msg)
{
    // todo: print something here?
}

void Raft::ApplyLog()
{
    assert(lastApplied_ <= commitIndex_);

    if (commitIndex_ != lastApplied_) {
        if (lastApplied_ + 1 == commitIndex_) {
            DEBUG("raft[%d] %s, term %d, apply log [%d]",
                  me_, RoleString(), currentTerm_, commitIndex_);
        }
        else {
            DEBUG("raft[%d] %s, term %d, apply log (%d, %d]",
                  me_, RoleString(), currentTerm_, lastApplied_, commitIndex_);
        }
    }

    for (int i = lastApplied_ + 1; i <= commitIndex_; i++) {
        ApplyMsg msg(i, log_.CommandAt(i));
        applyCallback_(msg);
    }
    lastApplied_ = commitIndex_;
}

void Raft::TickOnElection()
{
    timeElapsed_++;
    if (timeElapsed_ >= randomizedElectionTimeout_) {
        ToCandidate(); // candidate -> candidate is OK
    }
}

void Raft::TickOnHeartbeat()
{
    timeElapsed_++;
    if (timeElapsed_ >= heartbeatTimeout_) {
        StartAppendEntries();
        ResetTimer();
    }
}

void Raft::ToFollower(bool termIncreased)
{
    if (role_ != kFollower) {
        DEBUG("raft[%d] %s -> follower", me_, RoleString());
    }

    role_ = kFollower;
    if (termIncreased) {
        votedFor_ = kVotedForNull;
        votesGot_ = 0;
    }
    ResetTimer();
}

void Raft::ToCandidate()
{
    if (role_ != kCandidate) {
        DEBUG("raft[%d] %s -> candidate", me_, RoleString());
    }

    role_ = kCandidate;
    currentTerm_++;
    votedFor_ = me_; // vote myself
    votesGot_ = 1;

    if (IsSingleNode()) {
        ToLeader();
    }
    else {
        ResetTimer();
        StartRequestVote();
    }
}

void Raft::ToLeader()
{
    DEBUG("raft[%d] %s -> leader", me_, RoleString());

    nextIndex_.assign(peerNum_, log_.LastLogIndex() + 1);
    matchIndex_.assign(peerNum_, kInitialMatchIndex);
    role_ = kLeader;
    ResetTimer();
}

void Raft::OnNewInputTerm(int term)
{
    if (currentTerm_ < term) {
        currentTerm_ = term;
        ToFollower(true);
    }
}

void Raft::ResetTimer()
{
    timeElapsed_ = 0;
    if (role_ != kLeader)
        randomizedElectionTimeout_ = randomGen_.Generate();
}
