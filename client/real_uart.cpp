#include "real_uart.h"

#include <asio/buffer.hpp>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

namespace tt09_levenshtein
{

RealUart::RealUart(asio::any_io_executor& executor, const std::filesystem::path& path)
    : m_serialPort(executor, path.string().c_str())
{
    m_serialPort.set_option(asio::serial_port::baud_rate(3000000));
    m_serialPort.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::type::one));
    m_serialPort.set_option(asio::serial_port::parity(asio::serial_port::parity::type::none));
    m_serialPort.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::type::none));
    m_serialPort.set_option(asio::serial_port::character_size(8));
}

asio::awaitable<void> RealUart::send(std::span<const std::byte> data)
{
    co_await asio::async_write(m_serialPort, asio::buffer(data), asio::use_awaitable);
}

asio::awaitable<void> RealUart::recv(std::span<std::byte> data)
{
    co_await asio::async_read(m_serialPort, asio::buffer(data), asio::use_awaitable);
}


} // namespace tt09_levenshtein