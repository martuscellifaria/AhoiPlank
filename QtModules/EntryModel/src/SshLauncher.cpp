#include "SshLauncher.h"
#include "EntryModel.h"
#include <print>

bool SshLauncher::launch(const Entry& entry,
                         const QString&,
                         const QString& terminal_cfg) {
    const QString terminal = TerminalLauncher::find_terminal(terminal_cfg);
    if (terminal.isEmpty()) {
        std::println(stderr, "ssh: no terminal found, set terminal in config.json");
        return false;
    }
    return TerminalLauncher::run_in_terminal_argv(terminal, { "ssh", entry.exec });
}
