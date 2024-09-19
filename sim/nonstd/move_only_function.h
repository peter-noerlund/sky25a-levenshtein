#pragma once

/**
 * Implementation of http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0288r7.html (any_invocable)
 * 
 * The following has not been implemented:
 *   - In-place construction
 *   - Noexcept variant
 *   - operator() doesn't follow constness of captured function
 */

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace nonstd
{

namespace _move_only_function
{

template<typename R, typename... ArgTypes>
struct vtable
{
    struct type
    {
        R(*invoke)(void*, ArgTypes...); // Invoke function from storage
        void(*move_construct)(void*, void*) noexcept; // Move construct storage from storage
        void(*destroy)(void*) noexcept; // Destroy storage
    };
};

template<typename R, typename... ArgTypes>
using vtable_t = typename vtable<R, ArgTypes...>::type;

template<typename F, typename R, typename... ArgTypes>
struct vtable_impl_alloc
{
    // storage is is really a F**
    static constexpr const vtable_t<R, ArgTypes...> value = {
        [](void* storage, ArgTypes... args) -> R
        {
            return std::invoke(**static_cast<std::add_pointer_t<F>*>(storage), std::forward<ArgTypes>(args)...);
        },
        [](void* storage, void* other) noexcept
        {
            *static_cast<std::add_pointer_t<F>*>(storage) = std::exchange(*static_cast<std::add_pointer_t<F>*>(other), nullptr);
        },
        [](void* storage) noexcept
        {
            delete *static_cast<std::add_pointer_t<F>*>(storage);
        }
    };
};

template<typename F, typename R, typename... ArgTypes>
constexpr static auto vtable_impl_alloc_v = vtable_impl_alloc<F, R, ArgTypes...>::value;

template<typename F, typename R, typename... ArgTypes>
struct vtable_impl_no_alloc
{
    // storage is really a F*
    static constexpr const vtable_t<R, ArgTypes...> value = {
        [](void* storage, ArgTypes... args) -> R
        {
            return std::invoke(*static_cast<std::add_pointer_t<F>>(storage), std::forward<ArgTypes>(args)...);
        },
        [](void* storage, void* other) noexcept
        {
            new(storage) F(std::move(*static_cast<std::add_pointer_t<F>>(other)));
        },
        [](void* storage) noexcept
        {
            static_cast<std::add_pointer_t<F>>(storage)->~F();
        }
    };
};

template<typename F, typename R, typename... ArgTypes>
constexpr static auto vtable_impl_no_alloc_v = vtable_impl_no_alloc<F, R, ArgTypes...>::value;

template<typename Signature>
class move_only_function;

template<typename R, typename... ArgTypes>
class move_only_function<R(ArgTypes...)>
{
private:
    using storage_type = std::aligned_storage_t<3 * sizeof(void*), alignof(void*)>;

public:
    using result_type = R;

    /**
     * Default constructor
     */
    move_only_function() noexcept = default;
    
    /**
     * Null constructor
     */
    move_only_function(std::nullptr_t) noexcept
    {
    }

    /**
     * Copy constructor disabled
     */
    move_only_function(const move_only_function&) = delete;

    /**
     * Move constructor
     */
    move_only_function(move_only_function&& other) noexcept
        : m_vtable(std::exchange(other.m_vtable, nullptr))
    {
        if (m_vtable)
        {
            m_vtable->move_construct(&m_storage, &other.m_storage);
            m_vtable->destroy(&other.m_storage);
        }
    }
    
    /**
     * Constructor for functions which doesn't require allocation
     */
    template<typename F, std::enable_if_t<
        !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>, move_only_function> && std::is_invocable_r_v<R, F, ArgTypes...> &&
        std::is_invocable_r_v<R, F, ArgTypes...> &&
        std::is_nothrow_move_constructible_v<F> && sizeof(F) <= sizeof(storage_type)
    , bool> = true>
    move_only_function(F&& func)
        : m_vtable(&vtable_impl_no_alloc_v<F, R, ArgTypes...>)
    {
        new(&m_storage) F(std::forward<F>(func));
    }

    /**
     * Constructor for functions which require allocation
     */
    template<typename F, std::enable_if_t<
        !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>, move_only_function> && std::is_invocable_r_v<R, F, ArgTypes...> &&
        std::is_invocable_r_v<R, F, ArgTypes...> &&
        (!std::is_nothrow_move_constructible_v<F> || sizeof(storage_type) < sizeof(F))
    , bool> = true>
    move_only_function(F&& func)
        : m_vtable(&vtable_impl_alloc_v<F, R, ArgTypes...>)
    {
        *reinterpret_cast<std::add_pointer_t<F>*>(&m_storage) = new F(std::forward<F>(func));
    }

    // In place construction omitted

    /**
     * Destructor
     */
    ~move_only_function()
    {
        if (m_vtable)
        {
            m_vtable->destroy(&m_storage);
        }
    }

    /**
     * Copy assignment deleted
     */
    move_only_function& operator=(const move_only_function&) = delete;

    /**
     * Move assignment
     */
    move_only_function& operator=(move_only_function&& rhs) noexcept
    {
        if (m_vtable)
        {
            m_vtable->destroy(&m_storage);
        }
        m_vtable = rhs.m_vtable;
        if (m_vtable)
        {
            m_vtable->move_construct(&m_storage, &rhs.m_storage);
            rhs.m_vtable->destroy(&rhs.m_storage);
            rhs.m_vtable = nullptr;
        }
        return *this;
    }

    /**
     * Null assignment
     */
    move_only_function& operator=(std::nullptr_t)
    {
        if (m_vtable)
        {
            m_vtable->destroy(&m_storage);
            m_vtable = nullptr;
        }
        return *this;
    }

    /**
     * Assignment operator for functions which doesn't require allocation
     */
    template<typename F, std::enable_if_t<
        !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>, move_only_function> && std::is_invocable_r_v<R, F, ArgTypes...> &&
        std::is_invocable_r_v<R, F, ArgTypes...> &&
        std::is_nothrow_move_constructible_v<F> && sizeof(F) <= sizeof(storage_type)
    , bool> = true>
    move_only_function& operator=(F&& func)
    {
        if (m_vtable)
        {
            m_vtable->destroy(&m_storage);
        }
        m_vtable = &vtable_impl_no_alloc_v<F, R, ArgTypes...>;
        new(&m_storage) F(std::forward<F>(func));
        return *this;
    }

    /**
     * Assignment operator for functions which require allocation
     */
    template<typename F, std::enable_if_t<
        !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>, move_only_function> && std::is_invocable_r_v<R, F, ArgTypes...> &&
        std::is_invocable_r_v<R, F, ArgTypes...> &&
        (!std::is_nothrow_move_constructible_v<F> || sizeof(storage_type) < sizeof(F))
    , bool> = true>
    move_only_function& operator=(F&& func)
    {
        if (m_vtable)
        {
            m_vtable->destroy(&m_storage);
        }
        m_vtable = &vtable_impl_alloc_v<F, R, ArgTypes...>;
        *reinterpret_cast<std::add_pointer_t<F>*>(&m_storage) = new F(std::forward<F>(func));
        return *this;
    }
    
    /**
     * Check if move_only_function contains a function
     */
    explicit operator bool() const noexcept
    {
        return m_vtable != nullptr;
    }

    /**
     * Execute function
     */
    R operator()(ArgTypes... args)
    {
        return m_vtable->invoke(&m_storage, std::forward<ArgTypes>(args)...);
    }

    /**
     * Swap with other function
     */
    void swap(move_only_function<R(ArgTypes...)>& other) noexcept
    {
        move_only_function<R(ArgTypes...)> temp(std::move(*this));
        *this = std::move(other);
        *other = std::move(temp);
    }

private:
    const vtable_t<R, ArgTypes...>* m_vtable = nullptr;
    storage_type m_storage;
};

} // namespace _move_only_function

template<typename Signature>
using move_only_function = typename _move_only_function::move_only_function<Signature>;

} // namespace nonstd

namespace std
{

/**
 * Free-form swap function
 */
template<typename Signature>
void swap(nonstd::move_only_function<Signature>&lhs, nonstd::move_only_function<Signature>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace std