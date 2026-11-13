#pragma once
#include "Entry.h"
#include <QString>

class EntryLauncher {
public:
    EntryLauncher() = default;
    EntryLauncher(const EntryLauncher&) = delete;
    EntryLauncher& operator=(const EntryLauncher&) = delete;
    EntryLauncher(EntryLauncher&&) noexcept = delete;
    EntryLauncher& operator=(EntryLauncher&&) noexcept = delete;
    virtual ~EntryLauncher() = default;

    virtual bool launch(const Entry& entry,
                        const QString& text_editor,
                        const QString& terminal) = 0;
};
