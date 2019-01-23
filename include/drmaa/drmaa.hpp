#pragma once

#include "drmaa.h"
#include <string>
#include <vector>
#include <mutex>

namespace Drmaa {

std::pair<bool, std::string> initialize();

void close_session();

class DrmaaJob {

public:
    DrmaaJob(
        const std::string& job_name
        , const std::string& job_path
        , const std::vector<std::string>& arguments
        , const std::string& working_dir
        , const std::string& queue
        , int running_time_hours
        , int n_cpus
        , int memory_gigabytes
            );


    ~DrmaaJob();

    bool Submit();

    bool Wait();

    std::string GetDiagnosis() const noexcept {
        return diagnosis_;
    }

    bool IsGood() const noexcept {
        return good_;
    }

    std::string GetJobId() const noexcept {
        return job_id_;
    }

    std::string GetJobName() const noexcept {
        return job_name_;
    }

protected:
    drmaa_job_template_t *jt_;
    std::string working_dir_;
    std::string job_name_;
    std::string job_path_;
    std::string job_id_;
    std::string queue_;
    int running_time_hours_;
    int n_cpus_;
    int memory_gigabytes_;
    bool good_;
    char diagnosis_[DRMAA_ERROR_STRING_BUFFER];
    std::vector<std::string> arguments_;
};

} // end of namespace Drmaa