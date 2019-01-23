#include "drmaa/drmaa.hpp"
#include <iostream>


using namespace std;

int main(int argc, char **argv) {
    // initialization
    auto init_res = Drmaa::initialize();
    if (init_res.first != DRMAA_ERRNO_SUCCESS) {
        cerr << "error initializing Drmaa API\n";
        cerr << init_res.second << endl;
        exit(1);
    }

    // create job
    auto job = Drmaa::DrmaaJob(
        "test1", /* job name */
        "/home/ubuntu/shared/test/test.sh",  /* job script */
        vector<string>{"argv0", "argv1"},  /* arguments to the script */
        "/home/ubuntu/shared/test",  /* working dir */
        "all.q", /*queue */
        1, /* 1 hour */
        1, /* 1 CPU */
        1 /* 1 G of memory*/
    );
    if (!job.IsGood()) {
        cerr << "failed to create job\n";
        goto failed;
    }

    // submit job
    if (job.Submit()) {
        cerr << job.GetJobId() << " has been submitted; now waiting for it to finish" << endl;
    } else {
        cerr << ";" << endl;
        goto failed;
    }

    // wait for job to finish
    if (job.Wait()) {
        cerr << "job finished successfully!\n";
    } else {
        cerr << "job failed\n";
        goto failed;
    }
    return 0;

    failed:
    cerr << "diagnosis: " << job.GetDiagnosis() << endl;
}