#pragma once
#include "drmaa/drmaa.hpp"
#include <string>
#include <vector>

namespace Drmaa {

class Watcher {
public:
    explicit Watcher(const std::string& bane);

    virtual ~Watcher();

    Watcher(const Watcher&) = delete;

    Watcher(Watcher&&);

    Watcher& operator=(const Watcher&) = delete;

    Watcher& operator=(Watcher&&);

    virtual bool RetrieveJob() = 0;

    void operator()();

protected:
    void prepare_job_template();

    Drmaa::DrmaaJob *job_ = nullptr;
    std::string name_;
    std::string job_name_;
    std::string working_dir_;
    std::string job_path_;
    std::string queue_ = "all.q";
    int running_time_hours_ = 24;
    int n_cpus_ = 24;
    int memory_gigabytes_ = 24;
    std::vector<std::string> arguments_;
};

}