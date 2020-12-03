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
#include <sys/stat.h>

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

bool is_number(const std::string &s) {
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](unsigned char c) {
                return !std::isdigit(c);
            }) == s.end();
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
    auto background_sign_pos = cmd_s.find_last_of('&');
    if (background_sign_pos != string::npos) {
        if (cmd_line[background_sign_pos - 1] != '|')
            cmd_s.insert(background_sign_pos, " ");
    }
    int pipe_position = cmd_s.find("|");
    if (pipe_position != string::npos) {
        return new PipeCommand(cmd_line);
    }
    char *args[21];
    _parseCommandLine(cmd_s.c_str(), args);
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
    if (strcmp(cmd_line, "") == 0 || strcmp(cmd_line, "\n") == 0
        || strcmp(cmd_line, "\r") == 0 || strcmp(cmd_line, "\t") == 0)
        return;
    this->jobsList.checkForFinishedJobs();
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
    cout << "";
}

SmallShell::~SmallShell() {

}


void ChangePromptCommand::execute() {
    this->shell->shell_name = this->new_prompt_name;

}

ChangePromptCommand::ChangePromptCommand(const char *cmdLine)
        : BuiltInCommand(cmdLine) {
    char *args[21];
    _parseCommandLine(cmdLine, args);
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
    this->original_cmd_line = new char[strlen(cmd_line) + 1];
    strcpy(original_cmd_line, cmd_line);
    this->shell = &SmallShell::getInstance();
}

Command::~Command() {}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << ::getpid() << std::endl;

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
    if (job == nullptr) {
        perror("smash error: job doesn't exists");
        return;
    }
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
    int max_jobID = -1;
    JobEntry *max_job = nullptr;
    for (auto job  : jobs) {
        if (job->ID > max_jobID && job->status == Stopped) {
            max_jobID = job->ID;
            max_job = job;
        }
    }
    *jobId = max_jobID;
    return max_job;
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
    this->regularPipe = cmd_line[pipe_position + 1] != '&';
    this->is_built_in =
            dynamic_cast<BuiltInCommand *>(first_command) != nullptr;
    if(dynamic_cast<BuiltInCommand *>(this->second_command) != nullptr)
        this->second_command = nullptr;
}

void PipeCommand::execute() {
    int outputFile = (this->regularPipe) ? 1 : 2;//1 = stdout : 2 = stderr.
    if (!first_command || !second_command) {
        perror("smash error: problem with pipe command");
        return;
    }

    int fd[2];
    pipe(fd);
    int pid = fork();

    int x;


    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    } else if (pid == 0) { //Son
        setpgrp();
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        // So the shell will wait to whatever it is.
        this->second_command->status = Foreground;
        this->second_command->execute();
        exit(0);

    } else { //Father
        x = dup(outputFile);
        setpgrp();
        if(dup2(fd[1], outputFile) == -1){
            perror("smash error: dup2 failed");
        }
        close(fd[0]);
        close(fd[1]);
        if (this->is_built_in) {
            this->first_command->execute();
        } else {
            int pid2 = fork();
            if (pid2 == 0) { // Second son
                setpgrp();
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
        close(outputFile);
        dup(x);
        if (second_command->status == Foreground) {
            this->shell->jobsList.setCurrentJob(this, pid, -1, false);
            waitpid(pid, NULL, WUNTRACED);
            delete this->shell->jobsList.currentJob;
            this->shell->jobsList.currentJob = nullptr;
        }

//        usleep(2000);
//        kill(getpid(),SIGINT);
    }
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
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
        perror("smash error: fork failed");
    } else if (pid == 0) { // Son
        setpgrp();
        ::execl("/bin/bash", "/bin/bash", "-c",
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
        auto job = this->shell->jobsList.getJobById(jobIdLocal);
        if (job == nullptr || job->status == Background)
            return -1;

    } else {
        shell->jobsList.getLastStoppedJob(&jobIdLocal);
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
        cout << "smash error: fg: invalid arguments" << std::endl;
        this->should_operate = false;
        return;
    }
    jobID = getMaxJobID(args);
    if (jobID == -1) { // Error in job id
        if (shell->jobsList.jobs.empty())
            cout << "smash error: fg: jobs list is empty" << std::endl;
        else
            cout << "smash error: fg: job-id " << args[1] << " does not exist"
                 << std::endl;
        this->should_operate = false;
        return;
    }

    this->should_operate = true;

}

void ForegroundCommand::execute() {
    if (!should_operate)
        return;
    auto job = shell->jobsList.getJobById(jobID);
    cout << job->command->original_cmd_line << " : " << job->pid << std::endl;
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
        cout << "smash error: bg: invalid arguments" << std::endl;
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
                     << std::endl;
            else //TODO: might be a problem here if no job was told to be bg'd.
                cout << "smash error: bg: job-id " << args[1]
                     << " is already running in the background" << std::endl;
        } else {
            cout << "smash error: bg: there is no stopped jobs to resume"
                 << std::endl;
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
    cout << job->command->original_cmd_line << " : " << job->pid << std::endl;
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
    if (args[1] == nullptr || args[1][0] != '-' ||
        args[2] == nullptr ||
        args[3] != nullptr) {//Error in number of arguments
        cout << "smash error: kill: invalid arguments" << std::endl;
        this->should_operate = false;
        return;
    }
//    std::string str(args[1]);
    string signum_as_str = string(args[1]);
    const char *part_of_signum_as_str = signum_as_str.substr(1,
                                                             signum_as_str.size() -
                                                             1).c_str();
    if (!is_number(part_of_signum_as_str)) {
        cout << "smash error: kill: invalid arguments" << std::endl;
        this->should_operate = false;
        return;
    }
    signum = atoi(part_of_signum_as_str);
    if (signum > 32) {
        cout << "smash error: kill: invalid arguments" << std::endl;
        this->should_operate = false;
        return;
    }
    auto job = shell->jobsList.getJobById(atoi(args[2]));
    if (job == nullptr) {
        cout << "smash error: kill: job-id " << args[2] << " does not exist"
             << std::endl;
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
             << std::endl;
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
        cout << "[" << job->ID << "] " << job->command->original_cmd_line
             << " : "
             << job->pid
             << " " << difftime(time(nullptr), job->start_time) << " secs"
             << status_string
             << std::endl;
    }

}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    this->shell->jobsList.checkForFinishedJobs();

}

void lsCommand::execute() {
    struct dirent **namelist;
    vector<string> files = vector<string>();
    int n;
    int i = -1;
    n = scandir(".", &namelist, NULL, alphasort);
    while (i < n) {
        i++;
        if (!namelist[i])
            continue;
        else if (strcmp(".", namelist[i]->d_name) == 0 ||
                 strcmp("..", namelist[i]->d_name) == 0)
            continue;
        std::cout << namelist[i]->d_name << std::endl;
    }
}

void pwdCommand::execute() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("smash error: getcwd failed");
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
        perror("smash error: getcwd failed");
        return;
    }

    if (this->path == "-") {
        this->path = string(shell->last_dir);
    }
    if (chdir(this->path.c_str()) != 0) {
        perror("smash error: chdir failed");
        return;
    }

    shell->last_dir = string(cwd);
}

TimeoutCommand::TimeoutCommand(const char *cmd_line) : Command(cmd_line) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    if (args[1] == nullptr || args[2] == nullptr) {
        cout << "smash error: timeout: invalid arguments" << std::endl;
        this->should_operate = false;
        return;
    }
    this->should_operate = true;
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
    if (this->should_operate) {
        auto final_time = this->inner_cmd->maxTime + time(nullptr);
        this->shell->timeouts.push_back(final_time);
        auto timeouts = this->shell->timeouts;
        sort(timeouts.begin(), timeouts.end());
        auto timeout_minimal = *timeouts.begin() - time(nullptr);
//        cout << "Minimal new time: " << timeout_minimal << std::endl;
        alarm(timeout_minimal);
        this->inner_cmd->execute();
    }

}

QuitCommand::QuitCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    this->shell->is_running = false;
    if (args[1] != NULL && strcmp(args[1], "kill") == 0) {
        cout << "smash: sending SIGKILL signal to "
             << this->shell->jobsList.jobs.size()
             << " jobs:" << std::endl;

        for (auto &job : shell->jobsList.jobs) {
            kill(job->pid, SIGKILL);
            if (job->pid2 != -1)
                kill(job->pid2, SIGKILL);

            cout << job->pid << ": " << job->command->original_cmd_line
                 << std::endl;
        }
    }
}

void QuitCommand::execute() {

}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(
        cmd_line) {
    std::string cmd_s(cmd_line);
    int b_pos = cmd_s.find_first_of('>');
    this->doubleBiggerThan = cmd_line[b_pos + 1] == '>';
    this->c = this->shell->CreateCommand(cmd_s.substr(0, b_pos).c_str());

    int offset = (this->doubleBiggerThan) ? 2 : 1;
    this->filename = _trim(cmd_s.substr(b_pos + offset));
}

void RedirectionCommand::prepare() {
    this->fd = dup(1);
    if(this->fd < 0)
        perror("smash error: dup failed");
    if(close(1) < 0)
        perror("smash error: close failed");
}

void RedirectionCommand::cleanup() {
    if(close(1) < 0)
        perror("smash error: close failed");
    if(dup(this->fd) < 0)
        perror("smash error: dup failed");
}

void RedirectionCommand::execute() {
    this->prepare();

    if (this->doubleBiggerThan) {
        if (open(this->filename.c_str(), O_APPEND | O_RDWR | O_CREAT,
                 S_IRWXU) == -1) {
            perror("smash error: open failed");
        }
    } else {
        if (open(this->filename.c_str(), O_TRUNC | O_RDWR | O_CREAT, S_IRWXU) ==
            -1) {
            perror("smash error: open failed");
        }
    }

    this->c->execute();
    this->cleanup();
}

cpCommand::cpCommand(const char *cmd_line, bool isBackground) : BuiltInCommand(
        cmd_line), isBackground(isBackground) {
    char *args[21];
    _parseCommandLine(cmd_line, args);
    this->exe = false;
    if (args[3]) {
        perror("smash error: cd: too many arguments");
    } else {
        this->src = string(args[1]);
        this->dst = string(args[2]);
        this->exe = true;

    }
}

void cpCommand::copySuccess() {
    std::cout << "smash: " << this->src << " was copied to " << this->dst
              << endl;
}

void errorCopy(const char *err) {
    perror(err);
    exit(1);
}

void cpCommand::execute() {//Son
    int pid = fork();
    int x;
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    } else if (pid == 0) {
        setpgrp();
        char fileBuff[MAX_BUFF];
        int src_fd, dst_fd, curr_fd = 1;
        src_fd = open(this->src.c_str(), O_RDONLY);
        if (src_fd == -1) {
            errorCopy("smash error: open failed src");
        }
        dst_fd = open(this->dst.c_str(), O_CREAT | O_TRUNC | O_RDWR,
                      S_IRWXU | S_IRWXG | S_IRWXO);
        if (dst_fd == -1) {
            close(src_fd);
            errorCopy("smash error: open failed dst");
        }
        while (curr_fd) {
            curr_fd = read(src_fd, &fileBuff, MAX_BUFF);
            if (curr_fd == -1) {
                close(src_fd);
                close(dst_fd);
                errorCopy("smash error: read failed");
            }
            int res = write(dst_fd, &fileBuff, curr_fd);
            if (res == -1) {
                close(dst_fd);
                close(src_fd);
                errorCopy("smash error: write failed");
            }
        }
        if (close(dst_fd) == -1) {
            close(src_fd);
            errorCopy("smash error: close failed");
        }
        if (close(src_fd) == -1)
            errorCopy("smash error: close failed");
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