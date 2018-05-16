//
// Created by frank on 18-4-20.
//

#ifndef RAFT_LOG_H
#define RAFT_LOG_H

#include <memory>
#include <vector>

#include <tinyev/noncopyable.h>
#include <jackson/Value.h>
#include <raft/Storage.h>
#include <raft/Struct.h>

namespace raft
{

class Log : ev::noncopyable
{
public:
    explicit
    Log(Storage* storage);

    int FirstIndex() const
    { return firstIndex_; }

    int FirstTerm() const
    { return log_[firstIndex_].term; }

    int LastIndex() const
    { return lastIndex_; }

    int LastTerm() const
    { return log_[lastIndex_].term; }

    int TermAt(int index) const
    { return log_[index].term; }

    const json::Value& CommandAt(int index) const
    { return log_[index].command; }

    IndexAndTerm LastIndexInTerm(int startIndex, int term) const;

    bool IsUpToDate(int index, int term) const;

    void Append(int term, const json::Value& command);

    void Overwrite(int firstIndex, const json::Value& entries);

    json::Value GetEntriesAsJson(int firstIndex, int maxEntries) const;

    bool Contain(int index, int term) const;

private:

    json::Value GetEntryAsJson(int index) const;

    void PutEntryFromJson(const json::Value& entry);

    struct Entry
    {
        Entry(int term_, const json::Value& command_)
                : term(term_), command(command_)
        {}

        Entry()
                : term(0), command(json::TYPE_NULL)
        {}

        int term;
        json::Value command; // from raft user or raft peer
    };

    Storage* storage_;
    int firstIndex_;
    int lastIndex_;
    std::vector<Entry> log_;
};

}

#endif //RAFT_LOG_H
