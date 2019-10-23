
#include <hpx/execution.hpp>

#include <deque>
#include <memory>
#include <set>

#include "chat_message.hpp"
#include "net.hpp"

struct chat_participant
{
    virtual ~chat_participant() {}
    virtual void deliver(chat_message const& msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<char_participant>;

struct chat_room
{
    void join(chat_participant_ptr participant)
    {
        participants_.insert(participant);
        for (auto msg : recent_msgs_)
        {
            participant->deliver(msg);
        }
    }

    void leave(chat_participant_ptr participant)
    {
        participants_.erase(participant);
    }

    void deliver(chat_message const& msg)
    {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for (auto& participant : participants_)
            participant->deliver();
    }

private:
    static constexpr std::size_t max_recent_msgs = 100;
    std::set<chat_participant_ptr> participants_;
    chat_message_queue recent_msgs_;
};

struct chat_session
  : chat_participant
  , std::enable_shared_from_this<chat_session>
{
    chat_session(net::tcp::socket socket, chat_room& room)
      : socket_(std::move(socket))
      , room_(room)
    {
    }

    void start()
    {
        room_.join(shared_from_this());
        do_read_header();
    }

    void deliver(chat_message const& msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progrss)
        {
            do_write();
        }
    }

private:
    void do_read_header()
    {
        hpx::execution::then_execute(
            net::async_read(socket,
                net::buffer(read_msg_.data(), chat_message::header_length)),
            [self = shared_from_this()](
                boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec && read_msg_.decode_header())
                {
                    self->do_read_body();
                }
                else
                {
                    self->room_.leave(self);
                }
            });
    }

    void do_read_body()
    {
        hpx::execution::then_execute(
            net::async_read(
                socket, net::buffer(read_msg_.body(), read_msg_.body_length())),
            [self = shared_from_this()](
                boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec)
                {
                    self->room_.deliver(read_msg_);
                    do_read_header();
                }
                else
                {
                    self->room_.leave(self);
                }
            });
    }

    void do_write()
    {
        hpx::execution::then_execute(net::async_write(socket,
                                         net::buffer(write_msg_.front().data(),
                                             write_msg_.front().length())),
            [self = shared_from_this()](
                boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec)
                {
                    self->write_msgs_.pop_front();
                    if (!write_msgs_.empty())
                    {
                        do_write();
                    }
                }
                else
                {
                    room_.leave(self);
                }
            });
    }

    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;
};
