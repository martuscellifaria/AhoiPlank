#include "DotfileLauncher.h"
#include "EntryModel.h"
#include <print>

bool DotfileLauncher::launch(const Entry& entry,
                             const QString& text_editor,
                             const QString& terminal_cfg) {
    const QString terminal = TerminalLauncher::find_terminal(terminal_cfg);
    if (terminal.isEmpty()) {
        std::println(stderr, "dotfiles: no terminal found, set terminal in config.json");
        return false;
    }

    const QString script = QStringLiteral("cd %1 && exec %2 .")
        .arg(TerminalLauncher::shell_quote(entry.exec), text_editor);

    return TerminalLauncher::run_in_terminal_argv(terminal, { "sh", "-c", script });
}
