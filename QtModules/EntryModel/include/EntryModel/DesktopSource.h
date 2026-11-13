#pragma once
#include "EntrySource.h"

class DesktopSource : public EntrySource {
public:
    QVector<Entry> load() override;

private:
    static bool parse_desktop(const QString& path, Entry& out);
};
