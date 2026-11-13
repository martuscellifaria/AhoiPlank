#pragma once
#include "EntrySource.h"

class SshSource : public EntrySource {
public:
    QVector<Entry> load() override;
};
