#include "client.h"

#include "bus.h"
#include "context.h"

namespace tt09_levenshtein
{

Client::Client(Context& context, Bus& bus) noexcept
    : m_context(context)
    , m_bus(bus)
{
}

asio::awaitable<void> Client::init()
{
    std::vector<std::byte> vectorMap(512, {});
    co_await m_bus.write(BaseBitvectorAddress, vectorMap);
}

} // namespace tt09_levenshtein