#include <fmt/printf.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

enum class Type
{
    Read,
    Write
};

struct Command
{
    Type type = Type::Read;
    std::uint32_t address = 0;
    std::uint8_t data = 0;
};

void run(const char* device, std::span<const Command> commands)
{
    int fd = open(device, O_RDWR);
    if (fd == -1)
    {
        throw std::system_error(errno, std::system_category());
    }

    termios options;
    if (tcgetattr(fd, &options) == -1)
    {
        throw std::system_error(errno, std::system_category());
    }

    cfsetspeed(&options, B3000000);
    cfmakeraw(&options);
    tcsetattr(fd, TCSANOW, &options);

    for (auto command : commands)
    {
        std::array<std::uint8_t, 4> data = {
            static_cast<std::uint8_t>((command.type == Type::Write ? 0x80 : 0x00) | ((command.address >> 16) & 0x7F)),
            static_cast<std::uint8_t>(command.address >> 8),
            static_cast<std::uint8_t>(command.address),
            static_cast<std::uint8_t>(command.data)
        };
        if (write(fd, data.data(), data.size()) == -1)
        {
            throw std::system_error(errno, std::system_category());
        }

        std::uint8_t buffer;
        if (read(fd, &buffer, sizeof(buffer)) == -1)
        {
            throw std::system_error(errno, std::system_category());
        }

        fmt::println("[{:02x} {:02x} {:02x} {:02x}] = {:02x}", data[0], data[1], data[2], data[3], buffer);
    }
}

int main (int argc, char** argv)
{
    if (argc < 3)
    {
        fmt::println(stderr, "Usage: uart_wb DEVICE COMMANDs...");
        return EXIT_FAILURE;
    }

    enum class State
    {
        Command,
        Address,
        Data
    };

    std::vector<Command> commands;
    State state = State::Command;

    Command command;
    for (int i = 2; i != argc; ++i)
    {
        std::string_view arg = argv[i];
        switch (state)
        {
            case State::Command:
                if (arg == "read")
                {
                    command.type = Type::Read;
                    state = State::Address;
                }
                else if (arg == "write")
                {
                    command.type = Type::Write;
                    state = State::Address;
                }
                else
                {
                    fmt::println(stderr, "Invalid command: {}", arg);
                    return EXIT_FAILURE;
                }
                break;

            case State::Address:
                command.address = static_cast<std::uint32_t>(std::strtoul(argv[i], nullptr, 0));
                if (command.type == Type::Read)
                {
                    commands.push_back(command);
                    command = {};
                    state = State::Command;
                }
                else
                {
                    state = State::Data;
                }
                break;

            case State::Data:
                command.data = static_cast<std::uint8_t>(std::strtoul(argv[i], nullptr, 0));
                commands.push_back(command);
                command = {};
                state = State::Command;
                break;
        }
    }

    try
    {
        run(argv[1], commands);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        fmt::println(stderr, "Caught exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}