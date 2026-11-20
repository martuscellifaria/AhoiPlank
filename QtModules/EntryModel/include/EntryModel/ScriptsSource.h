#pragma once
#include "EntrySource.h"

class ScriptsSource : public EntrySource {
public:
    QVector<Entry> load() override;
};
