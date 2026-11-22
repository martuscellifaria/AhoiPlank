#pragma once
#include "EntrySource.h"

class BookmarksSource : public EntrySource {
public:
    QVector<Entry> load() override;
    static bool generate_bookmarks_dotfile();
};
