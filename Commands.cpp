#include <algorithm>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <signal.h>
#include <dirent.h>
#include<stdio.h>
#include<sys/dir.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = string(cmd_line);
    int pipe_position = cmd_s.find("|");
    if (pipe_position != string::npos) {
        return new PipeCommand(cmd_line);
    }
    char *args[21];
    _parseCommandLine(cmd_line, args);
    // Built in commands
    if (strcmp(args[0], "chprompt") == 0) {
        return new ChangePromptCommand(cmd_line);
    } else if (strcmp(args[0], "ls") == 0) {
        return new lsCommand(cmd_line, NULL);
    } else if (strcmp(args[0], "showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (strcmp(args[0], "fg") == 0) {
        return new ForegroundCommand(cmd_line);
    } else if (strcmp(args[0], "bg") == 0) {
        return new BackgroundCommand(cmd_line);
    } else if (strcmp(args[0], "kill") == 0) {
        return new KillCommand(cmd_line);
    } else if (strcmp(args[0], "jobs") == 0) {
        return new JobsCommand(cmd_line);
    } else if (strcmp(args[0], "pwd") == 0) {
        return new pwdCommand(cmd_line, NULL);
    } else if (strcmp(args[0], "cd") == 0) {
        return new cdCommand(cmd_line, NULL);
    }


    // If didn't find any built in commands - treating it as external command
    return new ExternalCommand(cmd_line);
    // For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    this->jobsList.checkForFinishedJobs();
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
//    cout << "Counter : " << this->jobsList.counter << "\n";
}

SmallShell::~SmallShell() {

}


void ChangePromptCommand::execute() {
    this->shell->shell_name = this->new_prompt_name;

}

ChangePromptCommand::ChangePromptCommand(const char *cmdLine)
        : BuiltInCommand(cmdLine) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    char *new_name = "smash";
    if (args[1]) {
        new_name = args[1];
    }
    this->new_prompt_name = new_name;
}


BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command::Command(
        cmd_line) {
    string line = std::string(cmd_line);
    std::replace(line.begin(), line.end(), '&', ' ');
    this->cmd_line = (char *) line.c_str();
}

Command::Command(const char *cmd_line) {
//    this->original_cmd_line = string(cmd_line).c_str();
    this->original_cmd_line = (char *) malloc(strlen(cmd_line) + 1);
    strcpy(original_cmd_line, cmd_line);
    // TODO: Need to clean memory of the string
    this->shell = &SmallShell::getInstance();
}

Command::~Command() {}

void ShowPidCommand::execute() {
    std::cout << ::getpid() << "\n";

}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(
        cmd_line) {

}


JobsList::JobsList() {
    this->counter = 0;
    currentJob = nullptr;
}

void JobsList::addJob(Command *cmd, int processID, bool isStopped) {
    checkForFinishedJobs();
    // Add usage to stopped.
    counter++;
    if (isStopped)
        this->jobs.push_back(
                new JobEntry(cmd, this->counter, processID, Stopped));
    else
        this->jobs.push_back(
                new JobEntry(cmd, this->counter, processID, Background));
}

void JobsList::checkForFinishedJobs() {
    int pid = 1;
    while (pid != 0 and pid != -1) {
        int status;
        pid = waitpid(-1, &status, WNOHANG);
        if (pid != 0 and pid != -1) {
            this->removeJobById(pid);
        }
    }
    this->setCurrentCounter();


}

void JobsList::setCurrentCounter() {
    int maxCounter = 0;
    for (int i = 0; i < this->jobs.size(); i++) {
        if (jobs[i]->ID > maxCounter)
            maxCounter = jobs[i]->ID;
    }
    this->counter = maxCounter;
}

void JobsList::removeJobById(int jobId) {
    JobEntry *job = getJobById(jobId);
    if (job == nullptr)
        // TODO: add handling for error here?
        ;
    jobs.erase(std::remove(jobs.begin(), jobs.end(), job), jobs.end());
    setCurrentCounter();

}

JobEntry *JobsList::getJobById(int jobId) {
    for (auto &job : this->jobs)
        if (job->ID == jobId)
            return job;
    return nullptr;
}

JobEntry *JobsList::getJobByProcessId(int processID) {
    for (auto &job : this->jobs)
        if (job->pid == processID)
            return job;
    return nullptr;
}

// Returns -1 in pointer and nullptr for job entry if didn't
// find any job.
JobEntry *JobsList::getLastJob(int *lastJobId) {
    for (auto job  : jobs) {
        if (job->ID == this->counter) {
            *lastJobId = job->ID;
            return job;
        }
    }
    *lastJobId = -1;
    return nullptr;
}

JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    for (auto job  : jobs) {
        if (job->ID == this->counter && job->status == Stopped) {
            *jobId = job->ID;
            return job;
        }
    }
    *jobId = -1;
    return nullptr;
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {

    string cmd_s = string(cmd_line);
    int pipe_position = cmd_s.find("|");
//    string first_command =
    int second_part_length = cmd_s.length() - pipe_position;
    Command *first_command = this->shell->CreateCommand(
            cmd_s.substr(0, pipe_position - 1).c_str());
    Command *second_command = this->shell->CreateCommand(
            cmd_s.substr(pipe_position + 1,
                         second_part_length).c_str());

}

void PipeCommand::execute() {

}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
//     TODO: To check here for background.
    string line = _trim(this->original_cmd_line);
    if (line.back() == '&')
        this->status = Background;
    std::replace(line.begin(), line.end(), '&', ' ');
    this->cmd_line = (char *) malloc(line.size() + 1);
    strcpy(this->cmd_line, line.c_str());
}

void ExternalCommand::execute() {
    pid_t pid = fork();
    if (pid == -1) {
        //TODO: Change problem handling
        cout << "Problem Forking, why?!!!";
    } else if (pid == 0) { // Son
        setpgrp();
        ::execl("/bin/bash", "bash", "-c",
                this->cmd_line, NULL);// this->args_without_start);
    } else { //Father
        if (this->status == Foreground) {
            this->shell->jobsList.setCurrentJob(this, pid, false);
            waitpid(pid, NULL, WUNTRACED);
            delete this->shell->jobsList.currentJob;
            this->shell->jobsList.currentJob = nullptr;
        } else {
            this->shell->jobsList.checkForFinishedJobs();
            this->shell->jobsList.addJob(this, pid, false);
        }
    }

}


int Command::getMaxJobID(char *const *args) const {
    int jobID = -1;
    if (args[1] != nullptr) {
        jobID = atoi(args[1]);
        if (this->shell->jobsList.getJobById(jobID) == nullptr)
            return -1;

    } else {
        shell->jobsList.getLastJob(&jobID);
    }
    return jobID;
}

int Command::GetMaxStoppedJobID(char *const *args) const {
    int jobIdLocal = -1;
    if (args[1] != nullptr) {
        jobIdLocal = atoi(args[1]);
        if (this->shell->jobsList.getJobById(jobIdLocal) == nullptr)
            return -1;

    } else {
        shell->jobsList.getLastJob(&jobIdLocal);
    }
    return jobIdLocal;
}

ForegroundCommand::ForegroundCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {
    char *args[21];
    for (auto &arg : args) {
        arg = nullptr;
    }
    _parseCommandLine(this->cmd_line, args);
    if (args[2] != nullptr) {//Error in number of arguments
        cout << "smash error: fg: invalid arguments" << "\n";
        this->should_operate = false;
        return;
    }
    jobID = getMaxJobID(args);
    if (jobID == -1) { // Error in job id
        if (shell->jobsList.jobs.empty())
            cout << "smash error: fg: jobs list is empty" << "\n";
        else
            cout << "smash error: fg: job-id " << args[1] << " does not exist"
                 << "\n";
        this->should_operate = false;
        return;
    }

    this->should_operate = true;

}

void ForegroundCommand::execute() {
    if (!should_operate)
        return;
    auto job = shell->jobsList.getJobById(jobID);
    cout << job->command->original_cmd_line << " : " << job->pid << "\n";
    kill(job->pid, SIGCONT);
    auto command_to_use = job->command;
    job->command = nullptr;
    shell->jobsList.removeJobById(jobID);
    this->shell->jobsList.setCurrentJob(command_to_use, job->pid, false);
    int p_status;
    waitpid(job->pid, &p_status, WUNTRACED);
    if (!WIFSTOPPED(p_status)) {
        delete this->shell->jobsList.currentJob;
        this->shell->jobsList.currentJob = nullptr;

    }
}

BackgroundCommand::BackgroundCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {
    char *args[21];
    for (auto &arg : args) {
        arg = nullptr;
    }
    _parseCommandLine(this->cmd_line, args);
    if (args[2] != nullptr) {//Error in number of arguments
        cout << "smash error: bg: invalid arguments" << "\n";
        this->should_operate = false;
        return;
    }
    jobID = GetMaxStoppedJobID(args);
    if (jobID == -1) { // Error in job id
        auto job = shell->jobsList.getJobById(atoi(args[1]));
        if (job == nullptr)
            cout << "smash error: bg: job-id " << args[1] << " does not exist"
                 << "\n";
        else //TODO: might be a problem here if no job was told to be bg'd.
            cout << "smash error: bg: job-id " << args[1]
                 << " is already running in the background" << "\n";

        this->should_operate = false;
        return;
    }
    this->should_operate = true;

}

void BackgroundCommand::execute() {
    if (!should_operate)
        return;
    auto job = shell->jobsList.getJobById(jobID);
    cout << job->command->original_cmd_line << " : " << job->pid << "\n";
    kill(job->pid, SIGCONT);
    job->status = Background;
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    char *args[21];
    for (auto &arg : args) {
        arg = nullptr;
    }
    _parseCommandLine(this->cmd_line, args);
    auto signum_str = string(args[1]);
    signum = atoi(signum_str.substr(1, signum_str.size() - 1).c_str());
    if (args[1] == nullptr ||
        args[2] == nullptr) {//Error in number of arguments
        cout << "smash error: kill: invalid arguments" << "\n";
        this->should_operate = false;
        return;
    }
    auto job = shell->jobsList.getJobById(atoi(args[2]));
    if (job == nullptr) {
        cout << "smash error: kill: job-id " << args[2] << " does not exist"
             << "\n";
        this->should_operate = false;
        return;
    }
    this->jobPID = job->pid;
    this->should_operate = true;

}

void KillCommand::execute() {
    kill(this->jobPID, signum);
    cout << "signal number " << signum << "was sent to pid " << jobPID << "\n";
}

bool jobscompareJobEntrys(JobEntry *first, JobEntry *second) {
    return (first->ID < second->ID);
}

void JobsCommand::execute() {
    auto jobs_v = this->shell->jobsList.jobs;
    sort(jobs_v.begin(), jobs_v.end(), jobscompareJobEntrys);
    for (auto &job : jobs_v) {
        string status_string = "";
        if (job->status == Stopped)
            status_string = " (stopped)";
        cout << job->ID << " " << job->command->original_cmd_line << " : "
             << job->pid
             << " " << difftime(time(nullptr), job->start_time) << " secs"
             << status_string
             << "\n";
    }

}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    this->shell->jobsList.checkForFinishedJobs();

}

void lsCommand::execute() {
    struct dirent **namelist;
    vector <string> files = vector<string>();
    int n;
    int i = 0;
    n = scandir(".", &namelist, NULL, alphasort);
    while (i < n) {
        std::cout << namelist[i]->d_name << std::endl;
        ++i;
    }
}

void pwdCommand::execute() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        //todo: Handle error(?)
        return;
    }
    std::cout << cwd << std::endl;
}

cdCommand::cdCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    this->exe = false;
    if (args[2]) {
        std::cout << "smash error: cd: too many arguments" << std::endl;
    } else if (strcmp(args[1], "-") == 0 && shell->last_dir.empty()) {
        std::cout << "smash error: cd: OLDPWD not set" << std::endl;
    } else {
        this->path = string(args[1]);
        this->exe = true;
    }
}

void cdCommand::execute() {
    if (!exe)
        return;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        //todo: Handle error(?)
        return;
    }

    if (this->path == "-") {
        this->path = string(shell->last_dir);
    }
    if (chdir(this->path.c_str()) != 0) {
        perror("nope");//todo: change to right error handling...
        return;
    }

    shell->last_dir = string(cwd);
}
