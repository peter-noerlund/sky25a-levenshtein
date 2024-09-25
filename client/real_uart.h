#pragma once

#include "uart.h"

#include <asio/any_io_executor.hpp>
#include <asio/serial_port.hpp>

#include <filesystem>

namespace tt09_levenshtein
{

class RealUart : public Uart
{
public:
    RealUart(asio::any_io_executor executor, const std::filesystem::path& path);

    asio::awaitable<void> send(std::span<const std::byte> data) override;
    asio::awaitable<void> recv(std::span<std::byte> buffer) override;

private:
    asio::serial_port m_serialPort;
};

} // namespace tt09_levenshtein