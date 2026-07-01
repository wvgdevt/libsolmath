/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <vector>
#include <cstddef>
#include <iostream>
#include <cassert>
#include <unordered_set>

#include "exception.h"
#include "logger.h"

namespace sol::math {
template<class P>
struct pointee_type {
    using type = P;
};

template<class U>
struct pointee_type<U*> {
    using type = U;
};

template<class U, class Deleter>
struct pointee_type<std::unique_ptr<U, Deleter> > {
    using type = U;
};

// Fast iteration class with key access, for pointers
template<class K, class T>
class svector_pointers {
    using object_type = typename pointee_type<std::remove_cvref_t<T> >::type;

    [[nodiscard]] static object_type* _ptr(T& _x) noexcept
    {
        if constexpr (std::is_pointer_v<std::remove_cvref_t<T> >)
        {
            return _x;
        }
        else if constexpr (requires { { _x.get() } -> std::convertible_to<object_type*>; })
        {
            return _x.get();
        }
        else
        {
            return &_x;
        }
    }

    [[nodiscard]] static object_type const* _ptr(T const& _x) noexcept
    {
        if constexpr (std::is_pointer_v<std::remove_cvref_t<T> >)
        {
            return _x;
        }
        else if constexpr (requires { { _x.get() } -> std::convertible_to<object_type const*>; })
        {
            return _x.get();
        }
        else
        {
            return &_x;
        }
    }

public:
    using index_map_t    = std::unordered_map<K, size_t>;        // NOLINT
    using const_iterator = typename index_map_t::const_iterator; // NOLINT

    void clear()
    {
        DEBUG_ASSERT(m_iteration_modes.empty(), math::exception, "Clear while iteration!");
        m_index_map.clear();
        m_vector.clear();
        m_vector_add_buffer.clear();
        m_set_remove_buffer_keys.clear();
        m_iteration_modes.clear();
        m_set_add_buffer_keys.clear();
    }

    bool contains(K const _key) const
    {
        if (m_set_remove_buffer_keys.contains(_key))
            return false;

        if (m_index_map.contains(_key))
            return true;

        return m_set_add_buffer_keys.contains(_key);
    }

    [[nodiscard]] object_type* try_get(K const _key)
    {
        return _try_get_impl(*this, _key);
    }

    [[nodiscard]] object_type const* try_get(K const _key) const
    {
        return _try_get_impl(*this, _key);
    }

    template<class Predicate>
        requires std::predicate<Predicate&, object_type const&>
    [[nodiscard]] object_type* find_if(Predicate&& _condition)
    {
        return _find_if_impl(*this, std::forward<Predicate>(_condition));
    }

    template<class Predicate>
        requires std::predicate<Predicate&, object_type const&>
    [[nodiscard]] object_type const* find_if(Predicate&& _condition) const
    {
        return _find_if_impl(*this, std::forward<Predicate>(_condition));
    }

    bool empty() const
    {
        return m_vector.empty();
    }

    class iteration_view;
    class const_iteration_view;

    [[nodiscard]] iteration_view mutable_iteration()
    {
        return iteration_view(*this, container_modification::allowed);
    }

    [[nodiscard]] const_iteration_view const_iteration() const
    {
        return const_iteration_view(*this);
    }

    template<typename Predicate>
    [[nodiscard]] bool none(Predicate&& _predicate) const
    {
        return std::ranges::none_of(m_vector, std::forward<Predicate>(_predicate));
    }

    template<class U>
        requires std::constructible_from<T, U&&>
    void emplace(U&& _x)
    {
        auto const key = _ptr(_x)->get_id();
        if (!m_iteration_modes.empty())
        {
            VERIFY(_modification_allowed(), math::exception, "emplace while const_iteration!");
            VERIFY(!m_index_map.contains(key), math::exception, "Key already exists!");
            _emplace_while_iteration(key, std::forward<U>(_x));
            return;
        }

        DEBUG_ASSERT(_ptr(_x) != nullptr, math::exception, "emplace null pointer in svector_pointers!");
        DEBUG_ASSERT(key == _ptr(_x)->get_id(), math::exception, "key does not match element id in svector_pointers!");
        _emplace_exception_free(key, std::forward<U>(_x));
    }

    std::vector<T> const& vector_unsafe() const { return m_vector; }
    [[nodiscard]] size_t size() const { return m_vector.size(); }

    T try_remove_extract(K const _key)
    {
        VERIFY(m_iteration_modes.empty(), math::exception, "remove_extract while iteration!");
        auto const it_index_map = m_index_map.find(_key);
        if (it_index_map != m_index_map.end())
            return _remove(it_index_map);
        return nullptr;
    }

    void erase(K const _key)
    {
        if (!m_iteration_modes.empty())
        {
            VERIFY(_modification_allowed(), math::exception, "erase while const_iteration!");
            DEBUG_ASSERT(m_index_map.contains(_key), math::exception, "_key not found!");
            _erase_while_iteration(_key);
            return;
        }

        auto const it_index_map = m_index_map.find(_key);
        VERIFY(it_index_map != m_index_map.end(), math::exception, "_key not found!");
        _remove(it_index_map);
    }

    void erase_if_present(K const _key)
    {
        if (!m_iteration_modes.empty())
        {
            if (!m_index_map.contains(_key) && !m_set_add_buffer_keys.contains(_key))
                return;

            VERIFY(_modification_allowed(), math::exception, "erase_if_present while const_iteration!");

            _erase_while_iteration(_key);
            return;
        }

        auto const it_index_map = m_index_map.find(_key);
        if (it_index_map != m_index_map.end())
            _remove(it_index_map);
    }

private:
    enum class container_modification { // NOLINT
        allowed,                        // NOLINT
        not_allowed,                    // NOLINT
    };

    template<class Self>
    [[nodiscard]] static auto _try_get_impl(Self& _self, K const _key)
    {
        using self_t = std::remove_reference_t<Self>;

        using return_t = std::conditional_t<
            std::is_const_v<self_t>,
            object_type const*,
            object_type*>;

        auto const it = _self.m_index_map.find(_key);
        if (it != _self.m_index_map.end())
        {
            DEBUG_ASSERT(it->second < _self.m_vector.size(), math::exception, "Out of bounds in try_get!");
            return static_cast<return_t>(_ptr(_self.m_vector[it->second]));
        }

        auto buffer_it = _self._find_add_buffer(_key);
        if (buffer_it != _self.m_vector_add_buffer.end())
        {
            return static_cast<return_t>(_ptr(buffer_it->second));
        }

        return static_cast<return_t>(nullptr);
    }

    template<class Self, class Predicate>
    [[nodiscard]] static auto _find_if_impl(Self& _self, Predicate&& _condition)
    {
        using self_t = std::remove_reference_t<Self>;

        using return_t = std::conditional_t<
            std::is_const_v<self_t>,
            object_type const*,
            object_type*>;

        for (auto& el: _self.m_vector)
        {
            auto* object = _ptr(el);

            DEBUG_ASSERT(object != nullptr, math::exception, "Null element in find_if!");

            if (std::invoke(_condition, std::as_const(*object)))
            {
                return static_cast<return_t>(object);
            }
        }

        for (auto& [key, el]: _self.m_vector_add_buffer)
        {
            auto* object = _ptr(el);

            DEBUG_ASSERT(object != nullptr, math::exception, "Null add-buffer element in find_if!");

            if (std::invoke(_condition, std::as_const(*object)))
            {
                return static_cast<return_t>(object);
            }
        }

        return static_cast<return_t>(nullptr);
    }

    template<class U>
        requires std::constructible_from<T, U&&>
    void _emplace_exception_free(K const _key, U&& _x)
    {
        VERIFY(m_index_map.emplace(_key, m_vector.size()).second, math::exception, "Key already exists!");
        m_vector.emplace_back(std::forward<U>(_x));
    }

    void _null_element(K const _key)
    {
        auto const it = m_index_map.find(_key);
        VERIFY(it != m_index_map.end(), math::exception, "Key not found in svector_pointers!");
        DEBUG_ASSERT(it->second < m_vector.size(), math::exception, "Out of bounds in _null_element!");
        m_vector[it->second] = nullptr;
    }

    template<class U>
        requires std::constructible_from<T, U&&>
    void _emplace_while_iteration(K const _key, U&& _x)
    {
        VERIFY(m_set_add_buffer_keys.emplace(_key).second, math::exception,
               "key already scheduled for emplace in svector_pointers!");
        VERIFY(!m_set_remove_buffer_keys.contains(_key), math::exception,
               "key already scheduled for erase in svector_pointers!");

        m_vector_add_buffer.emplace_back(_key, std::forward<U>(_x));
    }

    void _erase_while_iteration(K const _key)
    {
        if (m_set_add_buffer_keys.erase(_key) > 0)
        {
            auto const add_it = _find_add_buffer(_key);
            if (add_it != m_vector_add_buffer.end())
            {
                m_vector_add_buffer.erase(add_it);
                return;
            }
        }

        VERIFY(m_set_remove_buffer_keys.emplace(_key).second, math::exception,
               "key already scheduled for erase in svector_pointers!");

        _null_element(_key);
    }

    void _process_buffer()
    {
        for (auto const el_key: m_set_remove_buffer_keys)
            erase(el_key);
        m_set_remove_buffer_keys.clear();

        for (auto& [key, el]: m_vector_add_buffer)
            _emplace_exception_free(key, std::move(el));
        m_vector_add_buffer.clear();
        m_set_add_buffer_keys.clear();
    }

    T _remove(const_iterator _it_index_map)
    {
        size_t const delete_pos = _it_index_map->second;

        while (!m_vector.empty() && _ptr(m_vector.back()) == nullptr)
            m_vector.pop_back();

        m_index_map.erase(_it_index_map);

        // The requested element was null and was removed while trimming the tail.
        if (delete_pos >= m_vector.size())
            return nullptr;

        // Take ownership of the element being erased.
        T erased = std::move(m_vector[delete_pos]);

        size_t last_pos = m_vector.size() - 1;
        if (last_pos != delete_pos)
        {
            T& last = m_vector[last_pos];

            object_type* moved = _ptr(last);
            DEBUG_ASSERT(moved != nullptr, math::exception, "Null last element in _remove!");

            K const moved_key      = moved->get_id();
            m_vector[delete_pos]   = std::move(last);
            m_index_map[moved_key] = delete_pos;
        }

        m_vector.pop_back();
        return erased;
    }

    [[nodiscard]] bool _modification_allowed() const
    {
        return std::ranges::none_of(m_iteration_modes, [](container_modification const _mode) {
            return _mode == container_modification::not_allowed;
        });
    }

    [[nodiscard]] auto _find_add_buffer(K const _key) const
    {
        return std::ranges::find_if(m_vector_add_buffer, [_key](auto const& _pair) {
            return _key == _pair.first;
        });
    }

    [[nodiscard]] auto _find_add_buffer(K const _key)
    {
        return std::ranges::find_if(m_vector_add_buffer, [_key](auto const& _pair) {
            return _key == _pair.first;
        });
    }

private:
    mutable std::vector<container_modification> m_iteration_modes;
    std::vector<T> m_vector;
    std::vector<std::pair<K, T> > m_vector_add_buffer;
    std::unordered_set<K> m_set_add_buffer_keys;
    std::unordered_set<K> m_set_remove_buffer_keys;
    std::unordered_map<K, size_t> m_index_map;
};

template<class K, class T>
class svector_pointers<K, T>::iteration_view {
public:
    explicit iteration_view(svector_pointers& _owner, container_modification const)
        : m_owner(&_owner)
    {
        m_owner_size_start = m_owner->m_vector.size();
        m_owner->m_iteration_modes.emplace_back(container_modification::allowed);
    }

    iteration_view(iteration_view const&)            = delete;
    iteration_view& operator=(iteration_view const&) = delete;
    iteration_view(iteration_view&&)                 = delete;
    iteration_view& operator=(iteration_view&&)      = delete;

    ~iteration_view() noexcept
    {
        assert(m_owner_size_start == m_owner->m_vector.size());
        assert(!m_owner->m_iteration_modes.empty());
        m_owner->m_iteration_modes.pop_back();
        if (m_owner && m_owner->m_iteration_modes.empty())
        {
            m_owner->_process_buffer();
        }
    }

    auto begin() const noexcept { return m_owner->m_vector.begin(); }
    auto end() const noexcept { return m_owner->m_vector.end(); }

private:
    size_t m_owner_size_start;
    svector_pointers* m_owner = nullptr;
};

template<class K, class T>
class svector_pointers<K, T>::const_iteration_view {
public:
    explicit const_iteration_view(svector_pointers const& _owner)
        : m_owner_const(&_owner)
    {
        m_owner_size_start = m_owner_const->m_vector.size();
        m_owner_const->m_iteration_modes.emplace_back(container_modification::not_allowed);
    }

    const_iteration_view(const_iteration_view const&)            = delete;
    const_iteration_view& operator=(const_iteration_view const&) = delete;
    const_iteration_view(const_iteration_view&&)                 = delete;
    const_iteration_view& operator=(const_iteration_view&&)      = delete;

    ~const_iteration_view() noexcept
    {
        if (!m_owner_const)
            return;

        assert(m_owner_size_start == m_owner_const->m_vector.size());
        assert(!m_owner_const->m_iteration_modes.empty());
        m_owner_const->m_iteration_modes.pop_back();
    }

    auto begin() const noexcept { return m_owner_const->m_vector.begin(); }
    auto end() const noexcept { return m_owner_const->m_vector.end(); }

private:
    size_t m_owner_size_start;
    svector_pointers const* m_owner_const = nullptr;
};

/*
 // TODO rework this as svector_pointers
template<class K, class T>
class svectorRef // NOLINT
{
public:
    T& at_index(size_t _i) { return m_vector.at(_i); }
    T& at(K const& _key) { return m_vector.at(m_index_map.at(_key)); }
    size_t size() const { return m_vector.size(); }
    bool count(K const& _key) const { return m_index_map.count(_key); }

    void push_back(T&& _x)
    {
        m_index_map.emplace(_x.get_id(), m_vector.size());
        m_vector.push_back(std::move(_x));
    }

    std::vector<T>& vector() { return m_vector; }
    void erase(K const& _key) { remove(m_index_map.at(_key)); }

protected:
    void remove(size_t _pos)
    {
        size_t const last_ind  = m_vector.size() - 1;
        auto const& delete_key = m_vector.at(_pos).get_id();
        m_index_map.erase(delete_key);

        if (last_ind != _pos)
        {
            auto const& last_key     = m_vector.at(last_ind).get_id();
            m_index_map.at(last_key) = _pos;
            std::swap(m_vector.at(last_ind), m_vector.at(_pos));
        }

        m_vector.resize(last_ind);
    }

private:
    std::vector<T> m_vector;
    std::unordered_map<K, size_t> m_index_map;
};*/
}
