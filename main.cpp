#include <iostream>
#include <string>

#include "process.hpp"
#include "ppipe.hpp"
#include "boolean.hpp"
#include "shell.hpp"

using namespace std;


int main(void)
{
    Shell shell;

    while(1)
    {
        shell.printPreviewMessage();

        auto smartCmdLine   = shell.getSmartCmdLine();
        const auto& cmdLine = smartCmdLine.getCmdLine();

        if (cmdLine == "")
            continue;
        if (cmdLine == Shell::exitCmd)
            break;

        // step2: process cmd or fall to step 1 (if error)
        // cmdLineAnalyzer

        // step3: execute cmd, pipe, or boolean and wait them (if in foreground)
        auto cmdProcess     = process::make_process(cmdLine);
        delete cmdProcess;
    }

    return 0;
}



