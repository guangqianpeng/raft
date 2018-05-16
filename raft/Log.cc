//
// Created by frank on 18-5-10.
//

#include <leveldb/db.h>

#include <raft/Log.h>

using namespace raft;

Log::Log(Storage* storage)
        : storage_(storage)
        , firstIndex_(storage->GetFirstIndex())
        , lastIndex_(storage->GetLastIndex())
{
    assert(firstIndex_ <= lastIndex_);
    size_t entryNum = lastIndex_ - firstIndex_ + 1;
    log_.reserve(entryNum);
    for (auto& entry: storage->GetEntries()) {
        PutEntryFromJson(entry);
    }

    assert(entryNum == log_.size());
}

IndexAndTerm Log::LastIndexInTerm(int startIndex, int term) const
{
    int index = std::min(startIndex, lastIndex_);
    for (; index >= FirstIndex(); index--) {
        if (TermAt(index) <= term)
            break;
    }
    return { index, TermAt(index) };
}

bool Log::IsUpToDate(int index, int term) const
{
    int lastLogTerm_ = LastTerm();
    if (lastLogTerm_ != term)
        return lastLogTerm_ < term;
    return lastIndex_ <= index;
}

void Log::Append(int term, const json::Value& command)
{
    log_.emplace_back(term, command);
    lastIndex_++;

    auto entry = GetEntryAsJson(lastIndex_);
    storage_->PrepareEntry(lastIndex_, entry);
    storage_->PutPreparedEntries();
    storage_->PutLastIndex(lastIndex_);
}

void Log::Overwrite(int firstIndex, const json::Value& entries)
{
    assert(firstIndex <= lastIndex_ + 1);

    log_.resize(firstIndex);
    for (const json::Value& entry: entries.getArray()) {
        PutEntryFromJson(entry);
        storage_->PrepareEntry(firstIndex++, entry);
    }
    if (entries.getSize() > 0) {
        storage_->PutPreparedEntries();
    }
    lastIndex_ = static_cast<int>(log_.size()) - 1;
    storage_->PutLastIndex(lastIndex_);
}

json::Value Log::GetEntriesAsJson(int firstIndex, int maxEntries) const
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

bool Log::Contain(int index, int term) const
{
    if (index > lastIndex_)
        return false;
    return log_[index].term == term;
}

json::Value Log::GetEntryAsJson(int index) const
{
    json::Value entry(json::TYPE_OBJECT);
    entry.addMember("term", log_[index].term);
    entry.addMember("command", log_[index].command);
    return entry;
}

void Log::PutEntryFromJson(const json::Value& entry)
{
    int term = entry["term"].getInt32();
    auto& command = entry["command"];
    log_.emplace_back(term, command);
}
