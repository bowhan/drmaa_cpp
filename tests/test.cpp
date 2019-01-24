#include "drmaa/watcher.hpp"
#include "signal.h"
#include <iostream>
#include <thread>

using namespace std;

once_flag g_dummy_once;

class DummyWatcher
    : public Drmaa::Watcher {
public:

    DummyWatcher(const string& name);

    bool RetrieveJob();
};


DummyWatcher::DummyWatcher(const string& name)
    :
    Drmaa::Watcher(name) {

}

bool DummyWatcher::RetrieveJob() {
    bool succ = false;
    std::call_once(g_dummy_once, [this, &succ]() {
        this->job_name_ = "test1";
        this->job_path_ = "/home/ubuntu/shared/test/test.sh";
        this->arguments_ = vector<string>{"argv0", "argv1"};
        this->working_dir_ = "/home/ubuntu/shared/test";
        succ = true;
    });
    return succ;
}

void ctrl_c_handler(int s) {
    fprintf(stderr, "!! [main] caught signal %d, clean up and exit...\n", s);
    Drmaa::close_session();
    exit(1);
}

int main(int argc, char **argv) {

    // initialization
    auto init_res = Drmaa::initialize();
    if (init_res.first != DRMAA_ERRNO_SUCCESS) {
        cerr << "error initializing Drmaa API\n";
        cerr << init_res.second << endl;
        exit(1);
    }

    // create jobs
    vector<thread> workers;

    // add dummy worker
    workers.emplace_back(DummyWatcher("dummy1"));
    workers.emplace_back(DummyWatcher("dummy2"));
    workers.emplace_back(DummyWatcher("dummy3"));
    workers.emplace_back(DummyWatcher("dummy4"));
    workers.emplace_back(DummyWatcher("dummy5"));

    // prepare for exit signal
    struct sigaction sig_handler;
    sig_handler.sa_handler = &ctrl_c_handler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, nullptr);

    // wait forever
    for (auto& thread: workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }


    return 0;
}