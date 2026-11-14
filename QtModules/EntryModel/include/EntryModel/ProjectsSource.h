#pragma once
#include "EntrySource.h"

class ProjectsSource : public EntrySource {
public:
    QVector<Entry> load() override;
    static void pin_project();
    static void unpin_project();
};
