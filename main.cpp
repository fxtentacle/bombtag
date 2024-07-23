#include <iostream>
#include <vector>
#include <filesystem>
#include <thread>
#include <csignal>
#include <cassert>
#include <cstring>
#include <algorithm>


extern char **environ;

struct RunMe {
    int timeout_sigterm = 30;
    int timeout_sigkill = 30;
    std::string target_pid;
    std::string target_name;
    char* name_ptr = nullptr;
    int max_new_name = 0;

    void print_usage() {
        std::cout << R"(bombtag will wait for one or more programs to voluntarily exit, or it'll forcibly terminate them, first with SIGTERM and then with SIGKILL
Usage: bombtag [options]
-t seconds     Timeout before SIGTERM (default 30)
-k seconds     After SIGTERM, timeout before SIGKILL (default 30)
-p pid         PID of the process that shall exit
-n name        Name of the process that shall exit
}
)" << std::endl;
    }

    bool parse_args(int argc, char **argv) {
        std::vector<std::string> args(argv + 1, argv + argc);
        if(args.empty()) {
            print_usage();
            return false;
        }
        for (auto i = args.begin(); i != args.end(); ++i) {
            if (*i == "-h" || *i == "--help") {
                print_usage();
                return false;
            } else if (*i == "-t") {
                timeout_sigterm = std::stoi(*++i);
            } else if (*i == "-k") {
                timeout_sigkill = std::stoi(*++i);
            } else if (*i == "-p") {
                target_pid = *++i;
            } else if (*i == "-n") {
                target_name = *++i;
            }
        }
        if(target_pid.empty() && target_name.empty()) {
            std::cout << "You need to specify either pid or name." << std::endl;
            return false;
        }
        {
            // move the environ so that we can use its RAM for changing our own process name
            int env_len = 0;
            while( environ[env_len] ) ++env_len;
            name_ptr = argv[0];
            max_new_name = environ[env_len - 1] + strlen(environ[env_len - 1]) - argv[0];
            char** new_environ = (char**)malloc(env_len * sizeof(char*));
            for(int i=0;i<env_len-1;++i) new_environ[i] = strdup(environ[i]);
            environ = new_environ;
        }
        return true;
    }

    void rename_myself(std::string suffix) {
        const std::string new_name = "bombtag"+(!target_pid.empty() ? " -p "+target_pid : "")+(!target_name.empty() ? " -n "+target_name : "")+suffix;
        strncpy(name_ptr, new_name.c_str(), max_new_name);
        name_ptr[max_new_name-1] = 0;
    }

    int wait_for_single_program_to_exit() {
        std::error_code ec;
        const std::filesystem::path check_me("/proc/"+target_pid+"/exe");
        while(timeout_sigterm-- > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            rename_myself(" ("+std::to_string(timeout_sigterm)+"s -> TERM)");
            auto path= std::filesystem::read_symlink(check_me, ec);
            if(ec && ec.value() == 2) return 0;
            if(!target_name.empty() && path.filename() != target_name) return 0;
        }
        kill(std::stoi(target_pid), SIGTERM);
        while(timeout_sigkill-- > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            rename_myself(" ("+std::to_string(timeout_sigkill)+"s -> KILL)");
            auto path= std::filesystem::read_symlink(check_me, ec);
            if(ec && ec.value() == 2) return 0;
            if(!target_name.empty() && path.filename() != target_name) return 0;
        }
        kill(std::stoi(target_pid), SIGKILL);
    }

    int spawn_copies_by_name() {
        std::error_code ec;
        auto my_path = std::filesystem::read_symlink("/proc/self/exe", ec);
        if(ec) return 1;

        for(auto const& folder : std::filesystem::directory_iterator{std::filesystem::path("/proc")}) {
            auto const& folder_name = folder.path().filename().string();
            auto is_not_number = [](char const& c) { return c < '0' || c > '9'; };
            if(std::any_of(folder_name.begin(), folder_name.end(), is_not_number)) continue;
            auto exe_path = std::filesystem::read_symlink(folder.path() / "exe", ec);
            if(ec) continue;
            if(exe_path.filename() != target_name) continue;
            std::cout << "Found PID " << folder_name << " with EXE " << target_name << std::endl;
            pid_t child_id = fork();
            if(child_id == 0) {
                target_pid = folder_name;
                return wait_for_single_program_to_exit();
            }
        }
    }

    int run() {
        if(!target_pid.empty()) return wait_for_single_program_to_exit();
        else return spawn_copies_by_name();
    }
};


int main(int argc, char **argv) {
    RunMe runme;
    if(!runme.parse_args(argc, argv)) return 1;
    return runme.run();
}
