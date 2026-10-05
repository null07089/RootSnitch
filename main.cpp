#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <cstring>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

static const char *TAG_LABEL   = "进程联网告警";
static const char *NOTIF_STYLE = "bigtext";
static const char *MTIO_FILTER = "/data/user/0/bin.mt.plus/files/mtio ";
static constexpr int MAX_QUEUE = 1024;
static constexpr int LOOP_SLEEP_SEC = 1;
static constexpr uid_t SHELL_UID = 2000;
static constexpr gid_t SHELL_GID = 2000;

static volatile sig_atomic_t sigchld_received = 0;

static void sigchld_handler(int) { sigchld_received = 1; }

static void reap_children() {
    if (!sigchld_received) return;
    sigchld_received = 0;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}
}

static std::string read_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string read_proc_line(const std::string &path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return {};
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return {};
    if (buf[n - 1] == '\n') n--;
    if (n > 0 && buf[n - 1] == '\r') n--;
    return std::string(buf, n);
}

static bool match_selinux(const std::string &label) {
    return label.rfind("u:r:su:s0",     0) == 0
        || label.rfind("u:r:ksu:s0",    0) == 0
        || label.rfind("u:r:magisk:s0", 0) == 0;
}

static bool has_socket_fd(int pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/fd";
    DIR *dp = opendir(path.c_str());
    if (!dp) return false;

    bool found = false;
    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        std::string link = path + "/" + entry->d_name;
        char buf[256];
        ssize_t n = readlink(link.c_str(), buf, sizeof(buf) - 1);
        if (n <= 0) continue;
        buf[n] = '\0';
        if (strncmp(buf, "socket:[", 8) == 0) { found = true; break; }
    }
    closedir(dp);
    return found;
}

static std::string get_proc_args(int pid) {
    std::string raw = read_file("/proc/" + std::to_string(pid) + "/cmdline");
    if (raw.empty()) return "unknown";
    for (auto &c : raw) if (c == '\0') c = ' ';
    if (!raw.empty() && raw.back() == ' ') raw.pop_back();
    return raw;
}

static void send_notification(const std::string &msg) {
    pid_t child = fork();
    if (child < 0) return;
    if (child > 0) return;

    if (setresgid(SHELL_GID, SHELL_GID, SHELL_GID) < 0) _exit(1);
    if (setresuid(SHELL_UID, SHELL_UID, SHELL_UID) < 0) _exit(1);

    int mypid = getpid();
    struct stat st;
    std::string myproc = "/proc/" + std::to_string(mypid);
    std::string tag = std::to_string(mypid);
    if (stat(myproc.c_str(), &st) == 0)
        tag += "_" + std::to_string(st.st_mtime);

    char *argv[] = {
        const_cast<char *>("/system/bin/cmd"),
        const_cast<char *>("notification"),
        const_cast<char *>("post"),
        const_cast<char *>("-t"),
        const_cast<char *>(TAG_LABEL),
        const_cast<char *>("-S"),
        const_cast<char *>(NOTIF_STYLE),
        const_cast<char *>(tag.data()),
        const_cast<char *>(msg.data()),
        nullptr
    };

    execv(argv[0], argv);
    _exit(127);
}

static bool is_numeric(const char *s) {
    if (!*s) return false;
    for (; *s; ++s) if (*s < '0' || *s > '9') return false;
    return true;
}

static std::vector<std::pair<int, std::string>> find_root_processes() {
    std::vector<std::pair<int, std::string>> result;
    DIR *dp = opendir("/proc");
    if (!dp) return result;

    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr) {
        if (!is_numeric(entry->d_name)) continue;
        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        struct stat st;
        std::string proc_path = "/proc/" + std::to_string(pid);
        if (stat(proc_path.c_str(), &st) != 0 || st.st_uid != 0) continue;

        std::string label = read_proc_line(proc_path + "/attr/current");
        if (!match_selinux(label)) continue;
        if (!has_socket_fd(pid)) continue;

        std::string key = std::to_string(pid) + "_" + std::to_string(st.st_ino);
        result.emplace_back(pid, std::move(key));
    }
    closedir(dp);
    return result;
}

static void run_setup() {
    pid_t child = fork();
    if (child < 0) return;
    if (child == 0) {
        const char *argv[] = {
            "/system/bin/sh", "-c",
            "pm list package 2>/dev/null | grep -q com.google.android.ext.services && "
            "cmd notification allow_assistant com.google.android.ext.services/android.ext.services.notification.Assistant",
            nullptr
        };
        execv(argv[0], const_cast<char *const *>(argv));
        _exit(127);
    }
    waitpid(child, nullptr, 0);
}

static std::string get_exe_dir() {
    char buf[256];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    buf[n] = '\0';
    std::string path(buf);
    auto pos = path.rfind('/');
    return (pos == std::string::npos) ? "." : path.substr(0, pos);
}

int main() {

    signal(SIGCHLD, sigchld_handler);
    run_setup();

    std::string disable_path = get_exe_dir() + "/disable";
    std::unordered_map<std::string, bool> seen;
    std::deque<std::string> queue;
    bool paused = false;

    for (;;) {
        reap_children();

        bool disabled = access(disable_path.c_str(), F_OK) == 0;
        if (disabled != paused) {
            paused = disabled;
        }
        if (paused) {
            sleep(LOOP_SLEEP_SEC);
            continue;
        }

        auto entries = find_root_processes();
        for (const auto &entry : entries) {
            int pid = entry.first;
            const std::string &key = entry.second;
            if (seen.count(key)) continue;

            std::string args = get_proc_args(pid);
            if (args.rfind(MTIO_FILTER, 0) == 0) continue;

            send_notification(std::to_string(pid) + " | " + args);
            seen[key] = true;
            queue.push_back(key);

            if (queue.size() > MAX_QUEUE) {
                seen.erase(queue.front());
                queue.pop_front();
            }
        }

        sleep(LOOP_SLEEP_SEC);
    }
}
