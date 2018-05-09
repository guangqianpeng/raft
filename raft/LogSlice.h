//
// Created by frank on 18-4-20.
//

#ifndef RAFT_LOGSLICE_H
#define RAFT_LOGSLICE_H

#include <memory>
#include <vector>

#include <jackson/Value.h>

class LogSlice: ev::noncopyable
{
public:
    LogSlice(): log_{ Entry() }
    {}

    int FirstLogIndex() const
    { return 0; }

    int FirstLogTerm() const
    { return log_[0].term; }

    int LastLogIndex() const
    { return lastIndex_; }

    int LastLogTerm() const
    { return log_[lastIndex_].term; }

    struct IndexAndTerm {
        int index;
        int term;
    };
    IndexAndTerm LastIndexInTerm(int startIndex, int term) const
    {
        int index = std::min(startIndex, lastIndex_);
        for (; index >= FirstLogIndex(); index--) {
            if (TermAt(index) <= term)
                break;
        }
        return { index, TermAt(index) };
    }

    int TermAt(int index) const
    { return log_[index].term; }

    json::Value CommandAt(int index) const
    { return log_[index].command; }

    bool IsUpToDate(int index, int term) const
    {
        int lastLogTerm_ = LastLogTerm();
        if (lastLogTerm_ != term)
            return lastLogTerm_ < term;
        return lastIndex_ <= index;
    }

    void Append(int term, const json::Value& command)
    {
        log_.emplace_back(term, command);
        lastIndex_++;
    }

    void Overwrite(int firstIndex, const json::Value& entries)
    {
        assert(firstIndex <= lastIndex_ + 1);

        log_.resize(firstIndex);
        for (const json::Value& v: entries.getArray()) {
            int term = v["term"].getInt32();
            auto& command = v["command"];
            log_.emplace_back(term, command);
        }
        lastIndex_ = static_cast<int>(log_.size()) - 1;
    }

    json::Value GetEntriesAsJson(int firstIndex, int maxEntries) const
    {
        json::Value entries(json::TYPE_ARRAY);

        int lastIndex = std::min(lastIndex_, firstIndex + maxEntries - 1);
        for (int i = firstIndex; i <= lastIndex; i++)
        {
            auto element = GetEntryAsJson(i);
            entries.addValue(element);
        }
        return entries;
    }

    json::Value GetEntryAsJson(int index) const
    {
        json::Value entry(json::TYPE_OBJECT);
        entry.addMember("term", log_[index].term);
        entry.addMember("command", log_[index].command);
        return entry;
    }

    bool Contain(int index, int term) const
    {
        if (index > lastIndex_)
            return false;
        return log_[index].term == term;
    }

private:
    int lastIndex_ = 0;

    struct Entry
    {
        Entry(int term_, const json::Value& command_)
                : term(term_)
                , command(command_)
        {}
        Entry()
                : term(0)
                , command(json::TYPE_NULL)
        {}

        int term;
        json::Value command; // from raft user or raft peer
    };

    std::vector<Entry> log_;
};


#endif //JRPC_LOGSLICE_H
