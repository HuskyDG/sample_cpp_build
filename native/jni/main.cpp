#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/mount.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <string_view>
#include <string>
#include <set>

struct mount_info {
    unsigned int id;
    unsigned int parent;
    dev_t device;
    std::string root;
    std::string target;
    std::string vfs_option;
    struct {
        unsigned int shared;
        unsigned int master;
        unsigned int propagate_from;
    } optional;
    std::string type;
    std::string source;
    std::string fs_option;
};

#define ssprintf snprintf
#define parse_int(s) atoi(s.data())

// based on mountinfo code from https://github.com/yujincheng08

using namespace std;

bool starts_with(const char *s, const char *ss) {
    const char *str = strstr(s,ss);
    return str != nullptr && str == s;
}

std::vector<mount_info> parse_mount_info(const char *pid) {
    char buf[4098] = {};
    ssprintf(buf, sizeof(buf), "/proc/%s/mountinfo", pid);
    std::vector<mount_info> result;
    FILE *fp = fopen(buf, "re");
    if (fp == nullptr) return result;

    while (fgets(buf, sizeof(buf), fp)) {
        string_view line = buf;
        int root_start = 0, root_end = 0;
        int target_start = 0, target_end = 0;
        int vfs_option_start = 0, vfs_option_end = 0;
        int type_start = 0, type_end = 0;
        int source_start = 0, source_end = 0;
        int fs_option_start = 0, fs_option_end = 0;
        int optional_start = 0, optional_end = 0;
        unsigned int id, parent, maj, min;
        sscanf(line.data(),
               "%u "           // (1) id
               "%u "           // (2) parent
               "%u:%u "        // (3) maj:min
               "%n%*s%n "      // (4) mountroot
               "%n%*s%n "      // (5) target
               "%n%*s%n"       // (6) vfs options (fs-independent)
               "%n%*[^-]%n - " // (7) optional fields
               "%n%*s%n "      // (8) FS type
               "%n%*s%n "      // (9) source
               "%n%*s%n",      // (10) fs options (fs specific)
               &id, &parent, &maj, &min, &root_start, &root_end, &target_start,
               &target_end, &vfs_option_start, &vfs_option_end,
               &optional_start, &optional_end, &type_start, &type_end,
               &source_start, &source_end, &fs_option_start, &fs_option_end);

        auto root = line.substr(root_start, root_end - root_start);
        auto target = line.substr(target_start, target_end - target_start);
        auto vfs_option =
                line.substr(vfs_option_start, vfs_option_end - vfs_option_start);
        ++optional_start;
        --optional_end;
        auto optional = line.substr(
                optional_start,
                optional_end - optional_start > 0 ? optional_end - optional_start : 0);

        auto type = line.substr(type_start, type_end - type_start);
        auto source = line.substr(source_start, source_end - source_start);
        auto fs_option =
                line.substr(fs_option_start, fs_option_end - fs_option_start);

        unsigned int shared = 0;
        unsigned int master = 0;
        unsigned int propagate_from = 0;
        if (auto pos = optional.find("shared:"); pos != std::string_view::npos) {
            shared = parse_int(optional.substr(pos + 7));
        }
        if (auto pos = optional.find("master:"); pos != std::string_view::npos) {
            master = parse_int(optional.substr(pos + 7));
        }
        if (auto pos = optional.find("propagate_from:");
                pos != std::string_view::npos) {
            propagate_from = parse_int(optional.substr(pos + 15));
        }
        mount_info mnt_entry;
        mnt_entry.id = id;
        mnt_entry.parent = parent;
        mnt_entry.device = static_cast<dev_t>(makedev(maj, min));
        mnt_entry.root = root;
        mnt_entry.vfs_option = vfs_option;
        mnt_entry.optional = {
            .shared = shared,
            .master = master,
            .propagate_from = propagate_from,
            };
        mnt_entry.root = root;
        mnt_entry.target = target;
        mnt_entry.type = type;
        mnt_entry.source = source;
        mnt_entry.fs_option = fs_option;

        result.emplace_back(mnt_entry);
    }
    return result;
}

int main(int argc, char *argv[])
{
    std::string arg1 = (argc > 1 && argv[1] != nullptr)? argv[1] : "";
    if (arg1 == "--post-fs-data" || arg1 == "--boot-complete") {
        struct stat st{};
        stat("/proc/self/exe", &st);
        
        std::set<std::string> targets;
    
        for (auto &info : parse_mount_info("self")) {
            if (info.device != st.st_dev)
                continue;
            // unmount init.rc overlay in post-fs-data and magisk tmpfs when boot-completed
            if ((arg1 == "--post-fs-data" && starts_with(info.root.data(), "/.magisk/")) || (arg1 == "--boot-complete" && info.root == "/")) {
                targets.insert(info.target);
            }
        }
        
        auto last_target = *targets.cbegin() + '/';
        for (auto iter = next(targets.cbegin()); iter != targets.cend();) {
            if (starts_with((*iter).data(), last_target.data())) {
                iter = targets.erase(iter);
            } else {
                last_target = *iter++ + '/';
            }
        }
    
        for (auto &s : targets) {
            // unmount all magisk tmpfs and bind mount
            umount2(s.data(), MNT_DETACH);
        }
    }
    
    return 0;
}
