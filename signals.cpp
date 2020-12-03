#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << std::endl;
    auto shell = &SmallShell::getInstance();
    auto currentJob = shell->jobsList.currentJob;
    if (currentJob != nullptr) {

        cout << "smash: process " << currentJob->pid << " was stopped" << std::endl;
        currentJob->status = Stopped;
        shell->jobsList.addJob(currentJob->command, currentJob->pid, true,
                               currentJob->ID, currentJob->pid2);
        // Shit for memory management.
        currentJob->command = nullptr;
        delete currentJob;
        shell->jobsList.currentJob = nullptr;
        kill(currentJob->pid, SIGSTOP);

        if (currentJob->pid2 != -1) {
            kill(currentJob->pid2, SIGSTOP);
        }
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << std::endl;
    auto shell = &SmallShell::getInstance();
    auto currentJob = shell->jobsList.currentJob;
    if (currentJob != nullptr) {
        cout << "smash: process " << currentJob->pid << " was killed" << std::endl;
        if (currentJob->pid2 != -1)
            kill(currentJob->pid2, SIGKILL);
        kill(currentJob->pid, SIGKILL);

        delete currentJob;
        shell->jobsList.currentJob = nullptr;
    }
}

void alarmHandler(int sig_num) {
    auto shell = &SmallShell::getInstance();
    auto currentJob = shell->jobsList.currentJob;
    cout << "smash: got an alarm" << std::endl;
//    perror(" ");
    if (currentJob != nullptr) {
        double diff_time = difftime(time(nullptr), currentJob->start_time);
        if (currentJob->command->timeoutcommand &&
            diff_time >= currentJob->command->maxTime) {
            cout << "smash: " << currentJob->command->original_cmd_line
                 << " timed out!" << std::endl;
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
                 << " timed out" << std::endl;
            kill(currentJob->pid, SIGKILL);
        }
    }
    shell->timeouts.erase(shell->timeouts.begin());
    if (!shell->timeouts.empty()) {
        auto timeout_minimal = *shell->timeouts.begin() - time(nullptr);
        while (timeout_minimal <= 0 && !shell->timeouts.empty()) {
            shell->timeouts.erase(shell->timeouts.begin());
//            cout << "Erased!" << std::endl;
            timeout_minimal = *shell->timeouts.begin() - time(nullptr);
        }
        if (shell->timeouts.empty())
            return;
//        cout << "Minimal Time: " << timeout_minimal << std::endl;
        alarm(timeout_minimal);
    }
}

