/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <algorithm>
#include <unordered_map>
#include <map>
#include <vector>
#include <cstddef>
#include <iostream>
#include <cassert>
#include "exception.h"
#include "logger.h"

namespace sol::math {
// Fast iteration class with key access, for pointers
template<class K, class T>
class svector {
public:
    using index_map_t    = std::unordered_map<K, size_t>;        // NOLINT
    using const_iterator = typename index_map_t::const_iterator; // NOLINT

    void clear()
    {
        DEBUG_ASSERT(m_iteration_modes.empty(), math::exception, "Clear while iteration!");
        m_index_map.clear();
        m_vector.clear();
        m_vector_add_buffer.clear();
        m_vector_remove_buffer.clear();
        m_iteration_modes.clear();
    }

    bool contains(K const _key) const { return m_index_map.contains(_key); }

    T& get(K const _key)
    {
        auto const it = m_index_map.find(_key);
        if (it != m_index_map.end())
        {
            DEBUG_ASSERT(it->second < m_vector.size(), math::exception, "Out of bounds in try_get!");
            return m_vector[it->second];
        }
        auto const buffer_it = std::find_if(m_vector_add_buffer.begin(), m_vector_add_buffer.end(),
                                            [_key](auto const& _el) { return _el.get_id() == _key; });
        if (buffer_it != m_vector_add_buffer.end())
            return *buffer_it;
        DEBUG_ASSERT(false, math::exception, "Key not found!");
    }

    class iteration_view;

    [[nodiscard]] iteration_view mutable_iteration()
    {
        return iteration_view(*this, container_modification::allowed);
    }

    [[nodiscard]] iteration_view const_iteration()
    {
        return iteration_view(*this);
    }

    //[[nodiscard]] iteration_view const_iteration() const
    //{
    //    return iteration_view(*this);
    //}

    template<typename Predicate>
    [[nodiscard]] bool none(Predicate&& _predicate) const
    {
        return std::ranges::none_of(m_vector, std::forward<Predicate>(_predicate));
    }

    void emplace(K const _key, T _x)
    {
        DEBUG_ONLY(assert(!m_index_map.contains(_key)););

        if (!m_iteration_modes.empty())
        {
            VERIFY(m_iteration_modes.back() == container_modification::allowed, math::exception,
                   "emplace while const_iteration!");
            _emplace_while_iteration(_key, std::move(_x));
            return;
        }
        m_index_map.emplace(_key, m_vector.size());
        m_vector.emplace_back(std::move(_x));
    }

    std::vector<T> const& vector_unsafe() const { return m_vector; }
    [[nodiscard]] size_t size() const { return m_vector.size(); }

    void erase(K const _key)
    {
        if (!m_iteration_modes.empty())
        {
            VERIFY(m_iteration_modes.back() == container_modification::allowed, math::exception,
                   "erase while const_iteration!");
            _erase_while_iteration(_key);
            return;
        }

        auto const it_index_map = m_index_map.find(_key);
        DEBUG_ASSERT(it_index_map != m_index_map.end(), math::exception, "_key not found!");
        _remove(it_index_map);
    }

    void erase_if_present(K const _key)
    {
        auto const it_index_map = m_index_map.find(_key);
        if (it_index_map != m_index_map.end())
        {
            if (!m_iteration_modes.empty())
            {
                VERIFY(m_iteration_modes.back() == container_modification::allowed, math::exception,
                       "erase_if_present while const_iteration!");
                _erase_while_iteration(_key);
                return;
            }
            _remove(it_index_map);
        }
    }

private:
    enum class container_modification { // NOLINT
        allowed,                        // NOLINT
        not_allowed,                    // NOLINT
    };

    void _emplace_while_iteration(K const _key, T _x)
    {
        DEBUG_ASSERT(!m_iteration_modes.empty() && m_iteration_modes.back() == container_modification::allowed,
                     math::exception, "modification not allowed!");
        m_vector_add_buffer.push_back(std::move(_x));

        // TODO do we need it?
        auto const it = std::ranges::find_if(m_vector_remove_buffer,
                                             [_key](K const _element_key) {
                                                 return _element_key == _key;
                                             });
        if (it != m_vector_remove_buffer.end())
            m_vector_remove_buffer.erase(it);
    }

    void _erase_while_iteration(K _key)
    {
        DEBUG_ASSERT(!m_iteration_modes.empty() && m_iteration_modes.back() == container_modification::allowed,
                     math::exception, "modification not allowed!");
        auto const it = std::ranges::find_if(m_vector_add_buffer,
                                             [_key](T const& _element) {
                                                 return _key == _element.get_id();
                                             });
        if (it != m_vector_add_buffer.end())
            m_vector_add_buffer.erase(it);
        else
            m_vector_remove_buffer.push_back(_key);
    }

    void _process_buffer()
    {
        for (auto const el_key: m_vector_remove_buffer)
            erase(el_key);
        m_vector_remove_buffer.clear();

        for (auto& el: m_vector_add_buffer)
            emplace(el.get_id(), std::move(el));
        m_vector_add_buffer.clear();
    }

    void _remove(const_iterator _it_index_map)
    {
        size_t const delete_pos = _it_index_map->second;
        m_index_map.erase(_it_index_map);

        DEBUG_ASSERT(delete_pos < m_vector.size(), math::exception, "delete pos out of range!");

        size_t last_pos = m_vector.size() - 1;
        if (last_pos != delete_pos)
        {
            K const moved_key      = m_vector[last_pos].get_id();
            m_vector[delete_pos]   = std::move(m_vector[last_pos]);
            m_index_map[moved_key] = delete_pos;
        }

        m_vector.pop_back();
    }

private:
    //mutable size_t m_iteration_depth                   = 0;
    //mutable container_modification m_modification_mode = container_modification::not_allowed;
    mutable std::vector<container_modification> m_iteration_modes;
    std::vector<T> m_vector;
    std::vector<T> m_vector_add_buffer;
    std::vector<K> m_vector_remove_buffer;
    std::unordered_map<K, size_t> m_index_map;
};

template<class K, class T>
class svector<K, T>::iteration_view {
public:
    explicit iteration_view(svector& _owner, container_modification const) noexcept
        : m_owner(&_owner)
    {
        m_owner_size_start = m_owner->m_vector.size();
        m_owner->m_iteration_modes.emplace_back(container_modification::allowed);
    }

    explicit iteration_view(svector& _owner) noexcept
        : m_owner(&_owner)
    {
        m_owner_size_start = m_owner->m_vector.size();
        m_owner->m_iteration_modes.emplace_back(container_modification::not_allowed);
    }

    iteration_view(iteration_view const&)            = delete;
    iteration_view& operator=(iteration_view const&) = delete;
    iteration_view(iteration_view&&)                 = delete;
    iteration_view& operator=(iteration_view&&)      = delete;

    ~iteration_view() noexcept
    {
        if (!m_owner)
            return;

        assert(m_owner_size_start == m_owner->m_vector.size());
        assert(!m_owner->m_iteration_modes.empty());
        m_owner->m_iteration_modes.pop_back();

        if (m_owner->m_iteration_modes.empty())
            m_owner->_process_buffer();
    }

    auto begin() const noexcept { return m_owner->m_vector.begin(); }
    auto end() const noexcept { return m_owner->m_vector.end(); }

private:
    size_t m_owner_size_start;
    svector* m_owner = nullptr;
    //svector const* m_owner_const = nullptr;
};
}
