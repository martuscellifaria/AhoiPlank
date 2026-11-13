// #pragma once
// #include "Entry.h"
// #include <QVector>
//
// class EntrySource {
// public:
//     EntrySource() = default;
//     virtual ~EntrySource() = default;
//     virtual QVector<Entry> load() = 0;
//
//     EntrySource(const EntrySource&) = delete;
//     EntrySource& operator=(const EntrySource&) = delete;
// };

#pragma once
#include "Entry.h"
#include <QVector>

class EntrySource {
public:
    EntrySource() = default;
    EntrySource(const EntrySource&) = delete;
    EntrySource& operator=(const EntrySource&) = delete;
    EntrySource(EntrySource&&) noexcept = delete;
    EntrySource& operator=(EntrySource&&) noexcept = delete;
    virtual ~EntrySource() = default;

    virtual QVector<Entry> load() = 0;
};
