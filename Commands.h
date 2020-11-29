#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

class SmallShell;

enum JobStatus {
    Foreground, Background, Stopped
};

class Command {
private:
// TODO: Add your data members
public:
    SmallShell *shell;

    char *cmd_line;
    char *original_cmd_line;
    bool should_operate;
    bool timeoutcommand;
    int maxTime;

    Command(const char *cmd_line);

    int getMaxJobID(char *const *args) const;

    int GetMaxStoppedJobID(char *const *args) const;

    virtual ~Command();

    virtual void execute() = 0;


    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    char *path;
    char *args_without_start[2];
    JobStatus status;

    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;

};

class TimeoutCommand : public Command {
public:
    int max_time;
    time_t start_time;

    TimeoutCommand(const char *cmd_line);

    virtual ~TimeoutCommand() {}

    void execute() override;

    Command *inner_cmd;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    QuitCommand(const char *cmd_line);

    virtual ~QuitCommand() {}

    void execute() override;
};

class CommandsHistory {
protected:
    class CommandHistoryEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:

    CommandsHistory();

    ~CommandsHistory() {}

    void addRecord(const char *cmd_line);

    void printHistory();
};

class HistoryCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    HistoryCommand(const char *cmd_line, CommandsHistory *history);

    virtual ~HistoryCommand() {}

    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {

private:
    char *new_prompt_name;
    // TODO: Add your data members
public:
    ChangePromptCommand(const char *cmdLine);

    virtual ~ChangePromptCommand() {}

    void execute() override;

};

class JobEntry {
public:
    Command *command;
    time_t start_time;
    int ID;
    int pid;
    JobStatus status;

    JobEntry(Command *cmd, int id, int processID, JobStatus status) {
        this->command = cmd;
        this->ID = id;
        this->pid = processID;
        this->status = status;
        start_time = time(nullptr);
        // TODO: Add start time.
    }
};

class JobsList {
private:

public:

    std::vector<JobEntry *> jobs;
    JobEntry *currentJob;
    int counter;
    // TODO: Add your data members

    JobsList();

    ~JobsList() {}

    void
    addJob(Command *cmd, int processID, bool isStopped = false, int jobId = -1);

    void setCurrentJob(Command *cmd, int processID, int jid,
                       bool isStopped = false) {
        this->currentJob = new JobEntry(cmd, jid, processID, Foreground);
    }

    void printJobsList();

    void checkForFinishedJobs();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    JobEntry *getJobByProcessId(int processID);

    void removeJobById(int jobId);

    void setCurrentCounter();

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line);

    virtual ~KillCommand() {}

    void execute() override;

    int jobPID;
    int signum;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    int jobID;

    ForegroundCommand(const char *cmd_line);

    virtual ~ForegroundCommand() {}

    void execute() override;

};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    int jobID;

    BackgroundCommand(const char *cmd_line);


    virtual ~BackgroundCommand() {}

    void execute() override;
};

// TODO: add more classes if needed 
// maybe ls, timeout ?

class SmallShell {
private:

    SmallShell() {
        this->shell_name = (char *) "smash";
    }

public:
    char *shell_name;
    JobsList jobsList;
    std::vector<JobsCommand *> timeouts;
    std::string last_dir;
    bool is_running = true;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);
    // TODO: add extra methods as needed
};

class lsCommand : public BuiltInCommand {
public:
    lsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(
            cmd_line) {}

    virtual ~lsCommand() override {}

    void execute() override;
};

class pwdCommand : public BuiltInCommand {
public:
    pwdCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(
            cmd_line) {}

    virtual ~pwdCommand() override {}

    void execute() override;
};

class cdCommand : public BuiltInCommand {
private:
    std::string path;
    bool exe;
public:
    cdCommand(const char *cmd_line, JobsList *jobs);

    virtual ~cdCommand() override {}

    void execute() override;
};

#endif //SMASH_COMMAND_H_
