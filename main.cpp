#include <iostream>
#include <sstream>
#include "shell.hpp"

int main(void)
{
    shell::Shell myshell;

    while(1)
    {
        myshell.printPreviewMessage();

        auto smartCmdLine = myshell.getSmartCmdLine();
        const auto& cmdLine = smartCmdLine.getCmdLine();

        if (cmdLine == "")
            continue;
        if (cmdLine == shell::Shell::exitCmd)
            break;
        if (cmdLine == shell::Shell::jobsCmd)
        {
            myshell.jobs();
            delete &cmdLine;
            continue;
        }

        if (myshell.isControlFlowCmd())
        {
            std::stringstream sstream(cmdLine);
            std::string cmd; size_t N;
            sstream >> cmd >> N;

            if (cmd == shell::Shell::fgCmd)
                myshell.fg(N);
            else
                myshell.bg(N);
        }
        else
        {
            auto typeCmdLine = analyze::analyzeCmdLine(cmdLine);
            auto taskWrapper = analyze::createTask(cmdLine, typeCmdLine);

            if (!taskWrapper)
                continue;

            auto [task, isForeground] = *taskWrapper;
            myshell.addTaskItem({task, isForeground, cmdLine, typeCmdLine});
        }
    }

    return 0;
}



