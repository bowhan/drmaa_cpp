#include "drmaa/pipeline.hpp"
#include <thread>
#include <chrono>
#include <drmaa/pipeline.hpp>


namespace Drmaa {

Watcher::Watcher(const std::string& name)
    :
    name_(name) {

}

Watcher::Watcher(Watcher&& other)
    :
    job_(other.job_)
    , name_(other.name_)
    , job_name_(other.job_name_)
    , working_dir_(other.working_dir_)
    , job_path_(other.job_path_)
    , queue_(other.queue_)
    , running_time_hours_(other.running_time_hours_)
    , n_cpus_(other.n_cpus_)
    , memory_gigabytes_(other.memory_gigabytes_)
    , arguments_(std::move(other.arguments_)) {
    other.job_ = nullptr;
}

Watcher& Watcher::operator=(Watcher&& other) {
    if (this != &other) {
        delete job_;
        job_ = other.job_;
        other.job_ = nullptr;
        name_ = other.name_;
        job_name_ = other.job_name_;
        working_dir_ = other.working_dir_;
        job_path_ = other.job_path_;
        queue_ = other.queue_;
        running_time_hours_ = other.running_time_hours_;
        n_cpus_ = other.n_cpus_;
        memory_gigabytes_ = other.memory_gigabytes_;
        arguments_ = std::move(other.arguments_);
    }
    return *this;
}

Watcher::~Watcher() {
    delete job_;
}

void Watcher::prepare_job_template() {
    delete job_;
    job_ = new DrmaaJob(
        job_name_
        , job_path_
        , arguments_
        , working_dir_
        , queue_
        , running_time_hours_
        , n_cpus_
        , memory_gigabytes_
    );
}

void Watcher::operator()() {
    for (;;) {
        /* get job first */
        fprintf(stderr, "[%s] Getting job to run ...\n", name_.c_str());
        if (this->RetrieveJob()) {
            /* prepare the template */
            fprintf(stderr, "[%s] preparing job ...\n", name_.c_str());
            this->prepare_job_template();
            /* submit job */
            fprintf(stderr, "[%s] submit job ...\n", name_.c_str());
            this->job_->Submit();
            /* waiting */
            fprintf(stderr, "[%s] wait for job to finish ...\n", name_.c_str());
            this->job_->Wait();
        } else {
            fprintf(stderr, "[%s] No job, continue to wait...\n", name_.c_str());
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }
}

} // end of namespace