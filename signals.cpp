#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z\n";
    auto shell = &SmallShell::getInstance();
    auto currentJob = shell->jobsList.currentJob;
    if (currentJob != nullptr) {

        cout << "smash: process " << currentJob->pid << " was stopped\n";
        currentJob->status = Stopped;
        shell->jobsList.addJob(currentJob->command, currentJob->pid, true,
                               currentJob->ID);
        // Shit for memory management.
        currentJob->command = nullptr;
        delete currentJob;
        shell->jobsList.currentJob = nullptr;
        kill(currentJob->pid, SIGSTOP);
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C\n";
    auto shell = &SmallShell::getInstance();
    auto currentJob = shell->jobsList.currentJob;
    if (currentJob != nullptr) {
        cout << "smash: process " << currentJob->pid << " was killed\n";
        kill(currentJob->pid, SIGKILL);
        delete currentJob;
        shell->jobsList.currentJob = nullptr;
    }
}

void alarmHandler(int sig_num) {
    auto shell = &SmallShell::getInstance();
    auto currentJob = shell->jobsList.currentJob;
    if (currentJob != nullptr) {
        double diff_time = difftime(time(nullptr), currentJob->start_time);
        if (currentJob->command->timeoutcommand &&
            diff_time >= currentJob->command->maxTime) {
            cout << "smash: " << currentJob->command->original_cmd_line
                 << " timed out\n";
            kill(currentJob->pid, SIGKILL);
            delete currentJob;
            shell->jobsList.currentJob = nullptr;
        }
    }
    for (auto &command :shell->jobsList.jobs) {
        currentJob = command;
        if (currentJob->command->timeoutcommand &&
            difftime(time(nullptr), currentJob->start_time) >=
            currentJob->command->maxTime) {
            cout << "smash: " << currentJob->command->original_cmd_line
                 << " timed out\n";
            kill(currentJob->pid, SIGKILL);
        }
    }

}

