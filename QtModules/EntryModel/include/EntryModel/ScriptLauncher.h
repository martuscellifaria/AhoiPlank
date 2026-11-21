#pragma once
#include "EntryLauncher.h"

class ScriptLauncher : public EntryLauncher {
public:
    bool launch(const Entry& entry,
                const QString& text_editor,
                const QString& terminal) override;
private:
    bool store_script(const Entry& entry);
};
