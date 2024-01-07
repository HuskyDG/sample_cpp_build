#include <vector>

#include <unistd.h>

#include <fcntl.h>

#include <signal.h>

#include <pthread.h>

#include <sys/ptrace.h>

#include <sys/wait.h>

#include <string>

#include <set>

using namespace std;

#define WPTEVENT(x)(x >> 16)

#define WEVENT(__status)(((__status) >> 16) & 0xff)

#define STOPPED_WITH(sig, event) WIFSTOPPED(status) && (status >> 8 == ((sig) | (event << 8)))

std::string get_program(int pid) {
    std::string path = "/proc/";
    path += std::to_string(pid);
    path += "/exe";
    constexpr
    const auto SIZE = 256;
    char buf[SIZE + 1];
    auto sz = readlink(path.c_str(), buf, SIZE);
    if (sz == -1) {
        return "";
    }
    buf[sz] = 0;
    return buf;
}

int trace_init_main(int argc, char *argv[]) {
    int status;
    std::set < pid_t > process;

    if (ptrace(PTRACE_SEIZE, 1, 0, PTRACE_O_TRACEFORK) == -1) {
        fprintf(stderr, "cannot trace init\n");
        return -1;
    }
    fprintf(stderr, "start tracing init\n");

    for (int pid;;) {
        while ((pid = waitpid(-1, & status, __WALL | __WNOTHREAD)) != 0) {
            if (pid == 1) {
                if (STOPPED_WITH(SIGTRAP, PTRACE_EVENT_FORK)) {
                    long child_pid;
                    ptrace(PTRACE_GETEVENTMSG, pid, 0, & child_pid);
                    fprintf(stderr, "init forked %ld\n", child_pid);
                }
                if (WIFSTOPPED(status)) {
                    ptrace(PTRACE_CONT, pid, 0, (WPTEVENT(status) == 0)? WSTOPSIG(status) : 0);
                }
                continue;
            }
            auto state = process.find(pid);
            if (state == process.end()) {
                fprintf(stderr, "new process %d attached\n", pid);
                process.emplace(pid);
                ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACEEXEC);
                ptrace(PTRACE_CONT, pid, 0, 0);
                continue;
            } else {
                if (STOPPED_WITH(SIGTRAP, PTRACE_EVENT_EXEC)) {
                    auto program = get_program(pid);
                    fprintf(stderr, "proc monitor: pid %d is running program %s\n", pid, program.c_str());
                    do {
                        for (int i = 1; i < argc; i++) {
                            if (program == argv[i]) {
                                kill(pid, SIGSTOP);
                                ptrace(PTRACE_CONT, pid, 0, 0);
                                waitpid(pid, & status, __WALL);
                                if (STOPPED_WITH(SIGSTOP, 0)) {
                                    fprintf(stdin, "%d\n", pid);
                                    ptrace(PTRACE_DETACH, pid, 0, SIGSTOP);
                                    status = 0;
                                }
                                break;
                            }
                        }
                    } while (false);
                } else {
                    fprintf(stderr, "process %d received unknown status\n", pid);
                }
                process.erase(state);
                if (WIFSTOPPED(status)) {
                    fprintf(stderr, "detach process %d\n", pid);
                    ptrace(PTRACE_DETACH, pid, 0, 0);
                }
            }
        }
    }
    return 0;
}
