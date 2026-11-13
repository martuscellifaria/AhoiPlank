#pragma once
#include "EntryLauncher.h"

class BookmarkLauncher : public EntryLauncher {
public:
    bool launch(const Entry& entry,
                const QString& text_editor,
                const QString& terminal) override;
};
