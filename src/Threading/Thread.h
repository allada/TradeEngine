#ifndef Thread_h
#define Thread_h

#include "Threader.h"

namespace Threading {

class Thread : virtual public Threader {
public:
    Thread(std::unique_ptr<std::thread> thread, const std::string& name)
        : thread_(std::move(thread))
        , name_(name) { }

    ~Thread()
    {
        DEBUG("%s Thread Destructed", name_.c_str());
    }

    const std::string& name() const override { return name_; };
    void join() override;

    ThreadId id() const override { return thread_->get_id(); }

protected:
    std::unique_ptr<std::thread> thread_;

private:
    std::string name_;

};

} /* Thread_h */

#endif /* Thread_h */
