#include "shmmqprocessor.hpp"
#include "errors.hpp"

ShmMqProcessor::ShmMqProcessor(const char* conf_path, Role role)
{
    shmmq = new ShmMQ(conf_path);
    exit_if(shmmq == NULL, "new ShmMQ");
    notify_fd_handler = new FifoFd(conf_path, role);
    exit_if(notify_fd_handler == NULL, "new NotifyFileHandler");
}

ShmMqProcessor::~ShmMqProcessor()
{
    delete shmmq;
    delete notify_fd_handler;
}

int ShmMqProcessor::get_notify_fd() const
{
    return notify_fd_handler->get_notify_fd();
}

int ShmMqProcessor::produce(const void *data, unsigned data_len)
{
    int ret = shmmq->enqueue(data, data_len);
    notify_fd_handler->notify_event();
    return ret;
}

int ShmMqProcessor::consume(void *buffer, unsigned buffer_size, unsigned &data_len)
{
    char temp_buffer;
    int ret = shmmq->dequeue(buffer, buffer_size, data_len);
    notify_fd_handler->receive_event();
    return ret;
}