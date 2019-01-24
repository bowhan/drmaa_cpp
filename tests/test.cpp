#include "drmaa/pipeline.hpp"
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

    // wait forever
    for (auto& thread: workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // close session
    Drmaa::close_session();
}