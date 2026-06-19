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
#include <unordered_set>

#include "exception.h"
#include "logger.h"

namespace sol::math {
// Fast iteration class with key access, for pointers
template<class K, class T>
class svector_pointers {
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

    T* try_get(K const _key) const
    {
        auto const it = m_index_map.find(_key);
        if (it != m_index_map.end())
        {
            DEBUG_ASSERT(it->second < m_vector.size(), math::exception, "Out of bounds in try_get!");
            return m_vector[it->second];
        }
        auto const buffer_it = std::find_if(m_vector_add_buffer.begin(), m_vector_add_buffer.end(),
                                            [_key](auto const* _el) { return _el->get_id() == _key; });
        if (buffer_it != m_vector_add_buffer.end())
            return *buffer_it;
        return nullptr;
    }

    class iteration_view;

    [[nodiscard]] iteration_view mutable_iteration()
    {
        return iteration_view(*this, container_modification::allowed);
    }

    [[nodiscard]] iteration_view const_iteration() const
    {
        return iteration_view(*this);
    }

    template<typename Predicate>
    [[nodiscard]] bool none(Predicate&& _predicate) const
    {
        return std::ranges::none_of(m_vector, std::forward<Predicate>(_predicate));
    }

    void emplace(K const _key, T* _x)
    {
        DEBUG_ONLY(assert(_x != nullptr););
        if (!m_iteration_modes.empty())
        {
            VERIFY(_modification_allowed(), math::exception, "emplace while const_iteration!");
            _emplace_while_iteration(_key, _x);
            return;
        }
        VERIFY(m_index_map.emplace(_key, m_vector.size()).second, math::exception, "Key already exists!");
        m_vector.emplace_back(_x);
    }

    std::vector<T*> const& vector_unsafe() const { return m_vector; }
    [[nodiscard]] size_t size() const { return m_vector.size(); }

    void erase(K const _key)
    {
        if (!m_iteration_modes.empty())
        {
            VERIFY(_modification_allowed(), math::exception, "erase while const_iteration!");
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
                VERIFY(_modification_allowed(), math::exception, "erase_if_present while const_iteration!");
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

    void _null_element(K const _key)
    {
        DEBUG_ASSERT(m_index_map.contains(_key), math::exception, "Key not found in svector_pointers!");
        auto const index = m_index_map[_key];
        m_vector[index]  = nullptr;
    }

    void _emplace_while_iteration(K const _key, T* _x)
    {
        DEBUG_ASSERT(_modification_allowed(), math::exception, "modification not allowed!");
        DEBUG_ASSERT(_x != nullptr, math::exception, "cannot emplace null in svector_pointers!");
        DEBUG_ASSERT(_key == _x->get_id(), math::exception, "key does not match element id in svector_pointers!");

        // Do it in debug because it will be heavy on large emplace buffers
        DEBUG_ASSERT(_find_add_buffer(_key) == m_vector_add_buffer.end(), math::exception,
                     "key already scheduled for emplace in svector_pointers!");

        m_vector_remove_buffer.erase(_key);
        m_vector_add_buffer.push_back(_x);
    }

    void _erase_while_iteration(K _key)
    {
        DEBUG_ASSERT(_modification_allowed(), math::exception, "modification not allowed!");

        auto const add_it = _find_add_buffer(_key);
        if (add_it != m_vector_add_buffer.end())
        {
            m_vector_add_buffer.erase(add_it);
            return;
        }

        m_vector_remove_buffer.emplace(_key);
        _null_element(_key);
    }

    void _process_buffer()
    {
        for (auto const el_key: m_vector_remove_buffer)
            erase(el_key);
        m_vector_remove_buffer.clear();

        for (auto* el: m_vector_add_buffer)
            emplace(el->get_id(), el);
        m_vector_add_buffer.clear();
    }

    void _remove(const_iterator _it_index_map)
    {
        size_t const delete_pos = _it_index_map->second;

        while (!m_vector.empty() && m_vector.back() == nullptr)
            m_vector.pop_back();

        m_index_map.erase(_it_index_map);

        // The requested element was null and was removed while trimming the tail.
        if (delete_pos >= m_vector.size())
            return;

        size_t last_pos = m_vector.size() - 1;
        if (last_pos != delete_pos)
        {
            T* moved = m_vector[last_pos];
            DEBUG_ASSERT(moved != nullptr, math::exception, "Null last element in _remove!");
            m_vector[delete_pos]         = moved;
            m_index_map[moved->get_id()] = delete_pos;
        }

        m_vector.pop_back();
    }

    [[nodiscard]] bool _modification_allowed() const
    {
        return std::ranges::none_of(m_iteration_modes, [](container_modification const _mode) {
            return _mode == container_modification::not_allowed;
        });
    }

    [[nodiscard]] auto _find_add_buffer(K const _key) const
    {
        return std::ranges::find_if(m_vector_add_buffer, [_key](T const* _element) {
            return _key == _element->get_id();
        });
    }

private:
    mutable std::vector<container_modification> m_iteration_modes;
    std::vector<T*> m_vector;
    std::vector<T*> m_vector_add_buffer;
    std::unordered_set<K> m_vector_remove_buffer;
    std::unordered_map<K, size_t> m_index_map;
};

template<class K, class T>
class svector_pointers<K, T>::iteration_view {
public:
    explicit iteration_view(svector_pointers& _owner, container_modification const) noexcept
        : m_owner(&_owner), m_owner_const(&_owner)
    {
        m_owner_size_start = m_owner->m_vector.size();
        m_owner_const->m_iteration_modes.emplace_back(container_modification::allowed);
    }

    explicit iteration_view(svector_pointers const& _owner) noexcept
        : m_owner_const(&_owner)
    {
        m_owner_size_start = m_owner_const->m_vector.size();
        m_owner_const->m_iteration_modes.emplace_back(container_modification::not_allowed);
    }

    iteration_view(iteration_view const&)            = delete;
    iteration_view& operator=(iteration_view const&) = delete;
    iteration_view(iteration_view&&)                 = delete;
    iteration_view& operator=(iteration_view&&)      = delete;

    ~iteration_view() noexcept
    {
        if (!m_owner_const)
            return;

        assert(m_owner_size_start == m_owner_const->m_vector.size());
        assert(!m_owner_const->m_iteration_modes.empty());
        m_owner_const->m_iteration_modes.pop_back();

        if (m_owner && m_owner->m_iteration_modes.empty())
            m_owner->_process_buffer();
    }

    auto begin() const noexcept { return m_owner_const->m_vector.begin(); }
    auto end() const noexcept { return m_owner_const->m_vector.end(); }

private:
    size_t m_owner_size_start;
    svector_pointers* m_owner             = nullptr;
    svector_pointers const* m_owner_const = nullptr;
};

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
};
}
