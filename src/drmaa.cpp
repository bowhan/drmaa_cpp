#include "drmaa/drmaa.hpp"
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <thread>

#ifndef MAX_TRIAL_FOR_NEW_WORKER
#define MAX_TRIAL_FOR_NEW_WORKER 3
#endif

/* 12 is the average time for a new EC2 instance to be created & become available in the cluster */
#ifndef WAIT_INTERVAL_MIN
#define WAIT_INTERVAL_MIN 5
#endif

namespace Drmaa {

std::once_flag _initialize_flag, _close_flag;

std::pair<bool, std::string> initialize() {
    char diagnosis[1024];
    int status;
    std::call_once(_initialize_flag, [&]() {
        status = drmaa_init(nullptr, diagnosis, 1024 - 1);
    });
    return std::make_pair(status, diagnosis);
}


void close_session() {
    std::call_once(_close_flag, []() {
        drmaa_exit(nullptr, 0);
    });
}


DrmaaJob::DrmaaJob(const std::string& job_name
                   , const std::string& job_path
                   , const std::vector<std::string>& arguments
                   , const std::string& working_dir
                   , const std::string& queue
                   , int running_time_hours
                   , int n_cpus
                   , int memory_gigabytes
                  )
    :
    working_dir_(working_dir)
    , job_path_(job_path)
    , arguments_(arguments)
    , job_name_(job_name)
    , queue_(queue)
    , running_time_hours_(running_time_hours)
    , n_cpus_(n_cpus)
    , memory_gigabytes_(memory_gigabytes)
    , good_(true)
    , jt_(nullptr)
    , job_id_("") {

    int step = 0;
    const char **job_argv = nullptr;

    /* prepare the job template */
    if (DRMAA_ERRNO_SUCCESS != drmaa_allocate_job_template(&jt_, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* job working directory */
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_WD, working_dir_.c_str(), diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* job script path */
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_REMOTE_COMMAND, job_path_.c_str(), diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* job arguments */
    if (!arguments_.empty()) {
        job_argv = (const char **) malloc(sizeof(char *) * arguments_.size());
        if (job_argv == nullptr) {
            good_ = false;
            ++step;
            goto failed;
        }
        for (int i = 0; i < arguments_.size(); ++i) {
            job_argv[i] = strdup(arguments_[i].c_str());
        }
        if (DRMAA_ERRNO_SUCCESS
            != drmaa_set_vector_attribute(jt_, DRMAA_V_ARGV, job_argv, diagnosis_, sizeof(diagnosis_) - 1)) {
            good_ = false;
            ++step;
            goto failed;
        }
    }

    /* job name */
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_JOB_NAME, job_name_.c_str(), diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* job queue */
    char queue_to_submit[24];
    sprintf(queue_to_submit, "-q %s", queue_.c_str());
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_NATIVE_SPECIFICATION, queue_to_submit, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* set running time */
    char h_rt[24];
    char s_rt[24];
    sprintf(h_rt, "-l h_rt=%d:0:0", running_time_hours_);
    sprintf(s_rt, "-l s_rt=%d:0:0", running_time_hours_);
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_NATIVE_SPECIFICATION, h_rt, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_NATIVE_SPECIFICATION, s_rt, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* set CPU usage */
    char cpu[24];
    sprintf(cpu, "-pe smp %d", n_cpus_);
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_NATIVE_SPECIFICATION, cpu, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* set memory */
    char mem[24];
    sprintf(mem, "-l mem_free=%dG", memory_gigabytes_);
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_NATIVE_SPECIFICATION, mem, diagnosis_, sizeof(diagnosis_ - 1))) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* set the validation to none, this is important otherwise the job will fail because "no suitable queues" */
    char validation_level[12];
    sprintf(validation_level, "-w n");
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_NATIVE_SPECIFICATION, validation_level, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* job stdout/err */
    if (DRMAA_ERRNO_SUCCESS != drmaa_set_attribute(jt_, DRMAA_JOIN_FILES, "y", diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    /* path for output */
    char jobout[1024];
    jobout[0] = ':';
    sprintf(jobout + 1, "%s/jobout", working_dir.c_str());
    if (DRMAA_ERRNO_SUCCESS
        != drmaa_set_attribute(jt_, DRMAA_OUTPUT_PATH, jobout, diagnosis_, sizeof(diagnosis_) - 1)) {
        good_ = false;
        ++step;
        goto failed;
    }

    failed:
    if (!good_) {
        fprintf(stderr, "[%s] constructing drmaa job failed at step (%d)\n", job_name_.c_str(), step);
    }

    /* free */
    for (int i = 0; i < arguments_.size(); ++i) {
        free((void *) job_argv[i]);
    }
    free(job_argv);
}

DrmaaJob::~DrmaaJob() {
    drmaa_delete_job_template(jt_, nullptr, 0);
}


bool DrmaaJob::Submit() {
    if (!good_) {
        return false;
    }
    char jobid[24];
    int sid = drmaa_run_job(jobid, sizeof(jobid) - 1, jt_, diagnosis_, sizeof(diagnosis_) - 1);
    if (DRMAA_ERRNO_SUCCESS == sid) {
        job_id_ = jobid;
        return true;
    } else if (DRMAA_PS_SYSTEM_ON_HOLD == sid) {
        // need to parse diagnosis_ to retrive the jobid
        char *s, *e;
        for (s = diagnosis_; *s < '0' || *s > '9'; ++s);
        for (e = s; *e >= '0' && *e <= '9'; ++e);
        job_id_ = std::string(s, e);
        return true;
    } else {
        good_ = false;
        return false;
    }
}

bool DrmaaJob::Wait() {
    if (!good_) {
        return false;
    }

    /* first check job status */
    int remote_ps;
    drmaa_job_ps(job_id_.c_str(), &remote_ps, diagnosis_, sizeof(diagnosis_) - 1);

    int stat;
    char jobidout[100];
    int wait_status;
    if (DRMAA_PS_QUEUED_ACTIVE == remote_ps || DRMAA_PS_RUNNING == remote_ps) {
        wait_status = drmaa_wait(job_id_.c_str()
                                 , jobidout
                                 , sizeof(jobidout) - 1
                                 , &stat
                                 , DRMAA_TIMEOUT_WAIT_FOREVER
                                 , nullptr
                                 , diagnosis_
                                 , sizeof(diagnosis_) - 1);
    } else {
        fprintf(stderr, "[%s] Unknown drmaa_job_ps: %d\n", job_name_.c_str(), remote_ps);
        return false;
    }

    // wait until new instance has been added to the queue
    /* TODO(bowhan): seems drmaa_wait never works if the job was not submitted in the same session */
    for (int trial = 0; DRMAA_ERRNO_INVALID_JOB == wait_status && trial < MAX_TRIAL_FOR_NEW_WORKER; ++trial) {
        /* wait for WAIT_INTERVAL_MIN */
        for (int waited = 0; waited < WAIT_INTERVAL_MIN; ++waited) {
            fprintf(stderr
                    , "\r[%s] Trial %d/%d: waiting for another %3d min"
                    , job_name_.c_str()
                    , trial + 1
                    , MAX_TRIAL_FOR_NEW_WORKER
                    , WAIT_INTERVAL_MIN - waited);
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
        wait_status = drmaa_wait(job_id_.c_str()
                                 , jobidout
                                 , sizeof(jobidout) - 1
                                 , &stat
                                 , DRMAA_TIMEOUT_WAIT_FOREVER
                                 , nullptr
                                 , diagnosis_
                                 , sizeof(diagnosis_) - 1);

    } // end of dealing with job submit to queue when there is no worker

    return DRMAA_ERRNO_SUCCESS == wait_status;
}

} // namespace