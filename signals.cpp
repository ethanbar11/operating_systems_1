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
        shell->jobsList.addJob(currentJob->command, currentJob->pid, true);
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
    // TODO: Add your implementation
}

