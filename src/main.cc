#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/bind/bind.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <iostream>

class Printer {
 public:
  Printer(boost::asio::io_context& io) : timer_(io, std::chrono::seconds(1)) {
    timer_.async_wait(boost::bind(&Printer::print, this));
  }
  ~Printer() { std::cout << "Final count is " << count_ << std::endl; }

  void print() {
    if (count_ < 5) {
      std::cout << "hello world" << std::endl;
      ++count_;
      timer_.expires_at(timer_.expiry() + std::chrono::seconds(1));
      timer_.async_wait(boost::bind(&Printer::print, this));
    }
  }

 private:
  boost::asio::steady_timer timer_;
  int count_ = 0;
};

class PrinterWithStrand {
 public:
  PrinterWithStrand(boost::asio::io_context& io)
      : strand_(boost::asio::make_strand(io)),
        timer1_(io, std::chrono::seconds(1)),
        timer2_(io, std::chrono::seconds(2)) {
    timer1_.async_wait(boost::asio::bind_executor(
        strand_, boost::bind(&PrinterWithStrand::Print1, this)));
    timer2_.async_wait(boost::asio::bind_executor(
        strand_, boost::bind(&PrinterWithStrand::Print2, this)));
  }

  void Print1() {
    if (count_ < 20) {
      std::cout << "Timer 1 count: " << count_ << std::endl;
      ++count_;
      timer1_.expires_at(timer1_.expiry() + std::chrono::seconds(2));
      timer1_.async_wait(boost::asio::bind_executor(
          strand_, boost::bind(&PrinterWithStrand::Print1, this)));
    }
  }
  void Print2() {
    if (count_ < 20) {
      std::cout << "Timer 2 count: " << count_ << std::endl;
      ++count_;
      timer2_.expires_at(timer2_.expiry() + std::chrono::seconds(1));
      timer2_.async_wait(boost::asio::bind_executor(
          strand_, boost::bind(&PrinterWithStrand::Print2, this)));
    }
  }

 private:
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::asio::steady_timer timer1_;
  boost::asio::steady_timer timer2_;
  int count_ = 0;
};

void print(const boost::system::error_code& e, boost::asio::steady_timer* t,
           int* count) {
  if (*count < 5) {
    std::cout << "hello world" << std::endl;
    ++(*count);
    t->expires_at(t->expiry() + std::chrono::seconds(1));
    t->async_wait(
        boost::bind(print, boost::asio::placeholders::error, t, count));
  }
}

int main() {
  int count = 0;
  boost::asio::io_context io;
  // boost::asio::steady_timer t(io, std::chrono::seconds(1));
  // t.async_wait(
  //     boost::bind(print, boost::asio::placeholders::error, &t, &count));

  // Printer p(io);
  PrinterWithStrand p(io);
  boost::thread t(boost::bind(&boost::asio::io_context::run, &io));
  io.run();
  t.join();
  std::cout << "Final count is " << count << std::endl;
  return 0;
}
