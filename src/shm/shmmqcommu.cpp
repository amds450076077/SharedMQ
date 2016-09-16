#include "shmmqcommu.hpp"
#include "errors.hpp"
#include <sys/epoll.h>

ShmMQCommu::ShmMQCommu(const char *conf_path, Role role)
{
    smp = new ShmMqProcessor(conf_path, role);
    exit_if(smp == NULL, "new ShmMqProcessor");
    unsigned shmsize = ConfigReader::getConfigReader(conf_path)->GetNumber("shm", "shmsize", 10240);
    buffer_blob.capacity = shmsize;
    buffer_blob.data = new char[buffer_blob.capacity];
    exit_if(buffer_blob.data == NULL, "new blob");
}

ShmMQCommu::~ShmMQCommu()
{
    delete buffer_blob.data;
    delete smp;
}

int ShmMQCommu::sendData(const void *data, unsigned data_len)
{
    int ret = smp->produce(data, data_len);
    return ret;
}

void ShmMQCommu::readDataUntilEmpty()
{
    int ret;
    FOREVER
    {
        ret = smp->consume(buffer_blob.data, buffer_blob.capacity, buffer_blob.len);
        if (ret != 0)
        {
            break;
        }
        call_back->do_poll(&buffer_blob);
    }
}

int ShmMQCommu::listen(SHM_CALLBACK *call_back)
{
    int poll_fd = epoll_create(10);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = smp->get_notify_fd();
    int ctl_ret = epoll_ctl(poll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);
    exit_if(ctl_ret < 0, "epoll_ctl");
    
    this->call_back = call_back;

    readDataUntilEmpty();

    struct epoll_event events[10];
    FOREVER
    {
        int nfds = epoll_wait(poll_fd, events, 10, 5);
        if (nfds == -1)
        {
            TELL_ERROR("epoll_wait.");
            continue;
        }
        if (nfds > 0)
        {
            if (events[0].data.fd == smp->get_notify_fd())
            {
                readDataUntilEmpty();
            }
            else
            {
                TELL_ERROR("impossible.");
            }
        }
    }
    return 0;
}