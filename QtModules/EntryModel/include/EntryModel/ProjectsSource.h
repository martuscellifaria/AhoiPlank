#pragma once
#include "Entry.h"
#include "EntrySource.h"

class ProjectsSource : public EntrySource {
public:
    QVector<Entry> load() override;
    static void pin_project();
    static void unpin_project(const QString& project_name = "");
    static void list_projects();
};
