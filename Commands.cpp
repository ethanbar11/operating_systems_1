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
#include <fcntl.h>

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
#define MAX_BUFF 1024

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
        args[i] = new char[s.length() + 1];
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
    if (cmd_s.find('>') != string::npos) {
        return new RedirectionCommand(cmd_line);
    }
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
    } else if (strcmp(args[0], "cp") == 0) {
        return new cpCommand(cmd_line, _isBackgroundComamnd(cmd_line));
    } else if (strcmp(args[0], "timeout") == 0) {
        return new TimeoutCommand(cmd_line);
    } else if (strcmp(args[0], "quit") == 0) {
        return new QuitCommand(cmd_line);
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
    this->original_cmd_line = new char[strlen(cmd_line) + 1];
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

void
JobsList::addJob(Command *cmd, int processID, bool isStopped, int jobID,
                 int processID2) {
    checkForFinishedJobs();
    // Add usage to stopped.
    if (jobID == -1) {
        counter++;
        jobID = counter;
    }
    if (isStopped)
        this->jobs.push_back(
                new JobEntry(cmd, jobID, processID, Stopped, processID2));
    else
        this->jobs.push_back(
                new JobEntry(cmd, jobID, processID, Background, processID2));
}

void JobsList::checkForFinishedJobs() {
    int pid = 1;
    while (pid != 0 and pid != -1) {
        int status;
        pid = waitpid(-1, &status, WNOHANG);
        if (pid != 0 and pid != -1) {
            auto job = getJobByProcessId(pid);
            if (job != nullptr) // Meaning its probably pipe left
                this->removeJobById(job->ID);
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
        if (job->pid == processID || job->pid2 == processID)
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
    first_command = this->shell->CreateCommand(
            cmd_s.substr(0, pipe_position).c_str());
    second_command = dynamic_cast<ExternalCommand *>(this->shell->CreateCommand(
            cmd_s.substr(pipe_position + 1,
                         second_part_length).c_str()));
    this->regularPipe = cmd_line[pipe_position + 1] == '&';
    this->is_built_in =
            dynamic_cast<BuiltInCommand *>(first_command) != nullptr;
    //todo: if c2 is built in command - c2 = null...
    //todo: if & in cmd_line change this->isBackground to true
}

void PipeCommand::execute() {
    if (!first_command || !second_command) {
        //todo: handle error.
        return;
    }

    int fd[2];
    pipe(fd);
    int pid = fork();
    int x;

    if (pid == 0) { //Son
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        // So the shell will wait to whatever it is.
        this->second_command->status = Foreground;
        this->second_command->execute();
        exit(0);

    } else { //Father
        x = dup(1);
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        if (this->is_built_in) {
            this->first_command->execute();
        } else {
            int pid2 = fork();
            if (pid2 == 0) { // Second son
                this->first_command->execute();
                exit(0);
            } else { //Still father
                if (second_command->status == Foreground) {
                    this->shell->jobsList.setCurrentJob(this, pid, -1, false,
                                                        pid2);
                    waitpid(pid2, NULL, WUNTRACED);
                    delete this->shell->jobsList.currentJob;
                    this->shell->jobsList.currentJob = nullptr;

                } else {
                    this->shell->jobsList.addJob(this, pid, false, -1, pid2);
                }
            }
        }
        close(1);
        dup(x);
        if (second_command->status == Foreground) {
            this->shell->jobsList.setCurrentJob(this, pid, -1, false);
            waitpid(pid, NULL, WUNTRACED);
            delete this->shell->jobsList.currentJob;
            this->shell->jobsList.currentJob = nullptr;
        }
    }
}
//    if (this->second_command->status != Background) {
//        //todo: handle correctly... (problem with jobs... ask ethan)
//        if (pid == 0) {
//            dup2(fd[0], 0);
//            close(fd[0]);
//            close(fd[1]);
//            this->second_command->execute();
//        } else {
//            x = dup(1);
//            dup2(fd[1], 1);
//            close(fd[0]);
//            close(fd[1]);
//            this->first_command->execute();
//            close(1);
//            dup(x);
//            waitpid(pid, NULL, WUNTRACED);
//        }
//        close(fd[0]);
//        close(fd[1]);
//    } else {
//        if (pid == 0) {
//            pid2 = fork();
//            if (pid2 == 0) {
//                dup2(fd[0], 0);
//                close(fd[0]);
//                close(fd[1]);
//                this->second_command->execute();
//            } else {
//                x = dup(1);
//                dup2(fd[1], 1);
//                close(fd[0]);
//                close(fd[1]);
//                this->first_command->execute();
//                close(1);
//                dup(x);
//                waitpid(pid, NULL, WUNTRACED);
//            }
//        } else {
////            wait(NULL);
//            waitpid(pid2, NULL, WUNTRACED);
////            cat Makefile|grep @
//        }


//    int fd[2];
//    pipe(fd);
//    int a = fork();
//    int b = fork();
//
//    if (a == 0) {
//        dup2(fd[0], 1);
//        close(fd[0]);
//        close(fd[1]);
//        this->first_command->execute();
//    } else if (b == 0) {
//        dup2(fd[1], 0);
//        close(fd[0]);
//        close(fd[1]);
//        this->c2->execute();
//    } else {
//        wait(NULL);
//        wait(NULL);
//        wait(NULL);
////        waitpid(a,NULL, WUNTRACED);
////        waitpid(b,NULL, WUNTRACED);
//    }
//    close(fd[0]);
//    close(fd[1]);


ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
//     TODO: To check here for background.
    string line = _trim(this->original_cmd_line);
    this->timeoutcommand = false;
    if (line.back() == '&')
        this->status = Background;
    else {
        this->status = Foreground;
    }
    std::replace(line.begin(), line.end(), '&', ' ');
    this->cmd_line = new char[line.size() + 1];
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
            this->shell->jobsList.setCurrentJob(this, pid, -1, false);
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
    if (job->pid2 != -1)
        kill(job->pid2, SIGCONT);

    auto command_to_use = job->command;
    job->command = nullptr;
    shell->jobsList.removeJobById(jobID);
    this->shell->jobsList.setCurrentJob(command_to_use, job->pid, jobID,
                                        false, job->pid2);
    int p_status;
    waitpid(job->pid, &p_status, WUNTRACED);
    if (job->pid2 != -1) {
        waitpid(job->pid2, &p_status, WUNTRACED);
    }
    if (!WIFSTOPPED(p_status)) {
        //TODO might be bug without handling pid2
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
        if (args[1] != nullptr) {
            auto job = shell->jobsList.getJobById(atoi(args[1]));
            if (job == nullptr)
                cout << "smash error: bg: job-id " << args[1]
                     << " does not exist"
                     << "\n";
            else //TODO: might be a problem here if no job was told to be bg'd.
                cout << "smash error: bg: job-id " << args[1]
                     << " is already running in the background" << "\n";
        } else {
            cout << "smash error: bg: there is no stopped jobs to resume\n";
        }

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
    if (job->pid2 != -1)
        kill(job->pid2, SIGCONT);

    job->status = Background;
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    char *args[21];
    for (auto &arg : args) {
        arg = nullptr;
    }
    _parseCommandLine(this->cmd_line, args);
    if (args[1] == nullptr ||
        args[2] == nullptr) {//Error in number of arguments
        cout << "smash error: kill: invalid arguments" << "\n";
        this->should_operate = false;
        return;
    }
//    std::string str(args[1]);
    signum = atoi(args[1]);

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
    if (should_operate) {
        kill(this->jobPID, signum);
        cout << "signal number " << signum << " was sent to pid " << jobPID
             << "\n";
    }
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
    vector<string> files = vector<string>();
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

cdCommand::cdCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(
        cmd_line) {
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

TimeoutCommand::TimeoutCommand(const char *cmd_line) : Command(cmd_line) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    max_time = atoi(args[1]);
    start_time = time(nullptr);
    int start_index = strlen(args[0]) + strlen(args[1]) + 2;
    const char *inner_cmd_string = string(cmd_line).substr(start_index,
                                                           strlen(cmd_line) -
                                                           start_index).c_str();
    this->inner_cmd = this->shell->CreateCommand(inner_cmd_string);
    this->inner_cmd->timeoutcommand = true;
    this->inner_cmd->original_cmd_line = this->original_cmd_line;
    this->inner_cmd->maxTime = max_time;
}


void TimeoutCommand::execute() {
    alarm(this->inner_cmd->maxTime);
    this->inner_cmd->execute();

}

QuitCommand::QuitCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    this->shell->is_running = false;
    if (args[1] != NULL && strcmp(args[1], "kill") == 0) {
        cout << "smash: sending SIGKILL signal to "
             << this->shell->jobsList.jobs.size()
             << " jobs:\n";

        for (auto &job : shell->jobsList.jobs) {
            kill(job->pid, SIGKILL);
            if (job->pid2 != -1)
                kill(job->pid2, SIGKILL);

            cout << job->pid << ": " << job->command->original_cmd_line << '\n';
        }
    }
}

void QuitCommand::execute() {

}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    std::string cmd_s(cmd_line);
    int b_pos = cmd_s.find_first_of('>');
    this->doubleBiggerThan = cmd_line[b_pos + 1] == '>';
    this->c = this->shell->CreateCommand(cmd_s.substr(0, b_pos).c_str());

    int offset = (this->doubleBiggerThan) ? 1 : 2;
    this->filename = cmd_s.substr(b_pos + offset);
}

void RedirectionCommand::prepare() {
    this->fd = dup(1);
    close(1);
}

void RedirectionCommand::cleanup() {
    close(1);
    dup(this->fd);
}

void RedirectionCommand::execute() {
    this->prepare();

    if (this->doubleBiggerThan) {
        if (open(this->filename.c_str(), O_APPEND | O_RDWR | O_CREAT, S_IRWXU) == -1) {
            perror("bassa lecha");//todo: change to correct error method.
        }
    } else {
        if (open(this->filename.c_str(), O_TRUNC | O_RDWR | O_CREAT, S_IRWXU) == -1) {
            perror("bassa lecha 2");//todo: change to correct error method
        }
    }

    this->c->execute();

    this->cleanup();
}

cpCommand::cpCommand(const char *cmd_line, bool isBackground) : BuiltInCommand(cmd_line), isBackground(isBackground) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    this->exe = false;
    if (args[3]) {
        std::cout << "smash error: cd: too many arguments" << std::endl;
    } else if (strcmp(args[1], "-") == 0 && shell->last_dir.empty()) {
        std::cout << "smash error: cd: OLDPWD not set" << std::endl;//todo: maybe to perror.
    } else {
        this->src = string(args[1]);
        this->dst = string(args[2]);
        this->exe = true;
    }

    this->srcExists();
}

void cpCommand::copySuccess() {
    std::cout << "smash: " << this->src << " was copied to " << this->dst << endl;
}

void errorCopy(const char *err) {
    perror(err);//todo: handle error.
    exit(1);
}

void cpCommand::srcExists() {
    DIR *dir = opendir(this->src.c_str());
    if (!dir) {
        errorCopy("smash: file not found");
    }
    closedir(dir);
}

void cpCommand::execute() {//Son
    int pid = fork();
    int x;

    if (pid < 0) {
        perror("smash: error: fork failed");//todo: handle error.
        return;
    } else if (pid == 0) {
        char fileBuff[MAX_BUFF];
        int src_fd, dst_fd, curr_fd = 1;
        src_fd = open(this->src.c_str(), O_RDONLY);
        if (src_fd == -1) {
            errorCopy("smash: open failed");
        }
        dst_fd = open(this->dst.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        if (dst_fd == -1) {
            close(src_fd);
            errorCopy("smash: open failed");
        }

        while (curr_fd) {
            curr_fd = read(src_fd, &fileBuff, MAX_BUFF);
            if (curr_fd == -1) {
                close(src_fd);
                close(dst_fd);
                errorCopy("smash: read failed");
            }
            int res = write(dst_fd, &fileBuff, curr_fd);
            if (res == -1) {
                close(dst_fd);
                close(src_fd);
                errorCopy("smash: write failed");
            }
        }
        if (close(dst_fd) == -1) {
            close(src_fd);
            errorCopy("smash: close failed");
        }
        if (close(src_fd) == -1)
            errorCopy("smash: close failed");
        this->copySuccess();
        exit(0);
    } else {//Father
        if (this->isBackground) {
            shell->jobsList.checkForFinishedJobs();
            shell->jobsList.addJob(this, pid);
        } else {
            this->shell->jobsList.setCurrentJob(this, pid, -1, false);
            waitpid(pid, NULL, WUNTRACED);
            delete this->shell->jobsList.currentJob;
            this->shell->jobsList.currentJob = nullptr;
        }
    }
}