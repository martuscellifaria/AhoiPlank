#pragma once
#include "EntrySource.h"

class DotfilesSource : public EntrySource {
public:
    QVector<Entry> load() override;
};
