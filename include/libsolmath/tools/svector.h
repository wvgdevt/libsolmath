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
enum class container_modification { // NOLINT
    allowed,                        // NOLINT
    not_allowed,                    // NOLINT
};

// Fast iteration class with key access, for pointers
template<class K, class T>
class svector_pointers {
public:
    using index_map_t    = std::unordered_map<K, size_t>;        // NOLINT
    using const_iterator = typename index_map_t::const_iterator; // NOLINT

    void clear()
    {
        DEBUG_ASSERT(m_iteration_depth == 0, math::exception, "Clear while iteration!");
        m_iteration_depth   = 0;
        m_modification_mode = container_modification::not_allowed;
        m_index_map.clear();
        m_vector.clear();
        m_vector_add_buffer.clear();
        m_vector_remove_buffer.clear();
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

    [[nodiscard]] iteration_view iteration(container_modification const _mod = container_modification::not_allowed)
    {
        if (m_iteration_depth == 0)
            m_modification_mode = _mod;
        return iteration_view(*this);
    }

    [[nodiscard]] iteration_view iteration() const
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
        DEBUG_ONLY(
            assert(_x != nullptr);
            assert(!m_index_map.contains(_key));
        );

        if (m_iteration_depth > 0)
        {
            _emplace_while_iteration(_key, _x);
            return;
        }
        m_index_map.emplace(_key, m_vector.size());
        m_vector.emplace_back(_x);
    }

    std::vector<T*> const& vector_unsafe() const { return m_vector; }
    [[nodiscard]] size_t size() const { return m_vector.size(); }

    void erase(K const _key)
    {
        if (m_iteration_depth > 0)
        {
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
            if (m_iteration_depth > 0)
            {
                _erase_while_iteration(_key);
                return;
            }
            _remove(it_index_map);
        }
    }

private:
    void _null_element(K const _key)
    {
        DEBUG_ASSERT(m_index_map.contains(_key), math::exception, "Key not found in svector_pointers!");
        auto const index = m_index_map[_key];
        m_vector[index]  = nullptr;
    }

    void _emplace_while_iteration(K const _key, T* _x)
    {
        VERIFY(m_modification_mode == container_modification::allowed, math::exception, "modification not allowed!");
        m_vector_add_buffer.push_back(_x);

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
        VERIFY(m_modification_mode == container_modification::allowed, math::exception, "modification not allowed!");
        auto const it = std::ranges::find_if(m_vector_add_buffer,
                                             [_key](T const* _element) {
                                                 return _key == _element->get_id();
                                             });
        if (it != m_vector_add_buffer.end())
            m_vector_add_buffer.erase(it);
        else
        {
            m_vector_remove_buffer.push_back(_key);
            _null_element(_key);
        }
    }

    void _process_buffer()
    {
        for (auto const el_key: m_vector_remove_buffer)
            erase(el_key);
        m_vector_remove_buffer.clear();

        for (auto* el: m_vector_add_buffer)
            emplace(el->get_id(), el);
        m_vector_add_buffer.clear();

        m_modification_mode = container_modification::not_allowed;
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
            T* const moved = m_vector[last_pos];
            DEBUG_ASSERT(moved != nullptr, math::exception, "Null last element in _remove!");
            m_vector[delete_pos]         = moved;
            m_index_map[moved->get_id()] = delete_pos;
        }

        m_vector.pop_back();
    }

private:
    mutable size_t m_iteration_depth           = 0;
    container_modification m_modification_mode = container_modification::not_allowed;
    std::vector<T*> m_vector;
    std::vector<T*> m_vector_add_buffer;
    std::vector<K> m_vector_remove_buffer;
    std::unordered_map<K, size_t> m_index_map;
};

template<class K, class T>
class svector_pointers<K, T>::iteration_view {
public:
    explicit iteration_view(svector_pointers& _owner) noexcept
        : m_owner(&_owner), m_owner_const(&_owner)
    {
        m_owner_size_start = m_owner->m_vector.size();
        ++m_owner->m_iteration_depth;
    }

    explicit iteration_view(svector_pointers const& _owner) noexcept
        : m_owner_const(&_owner)
    {
        m_owner_size_start = m_owner_const->m_vector.size();
        ++m_owner_const->m_iteration_depth;
    }

    iteration_view(iteration_view const&)            = delete;
    iteration_view& operator=(iteration_view const&) = delete;

    iteration_view(iteration_view&& _other) noexcept
        : m_owner_size_start(_other.m_owner_size_start),
          m_owner(std::exchange(_other.m_owner, nullptr)),
          m_owner_const(std::exchange(_other.m_owner_const, nullptr))
    {}

    ~iteration_view() noexcept
    {
        if (!m_owner_const)
            return;

        assert(m_owner_size_start == m_owner_const->m_vector.size());
        assert(m_owner_const->m_iteration_depth > 0);
        --m_owner_const->m_iteration_depth;

        if (m_owner && m_owner->m_iteration_depth == 0)
            m_owner->_process_buffer();
    }

    auto begin() const noexcept { return m_owner_const->m_vector.begin(); }
    auto end() const noexcept { return m_owner_const->m_vector.end(); }

private:
    size_t m_owner_size_start;
    svector_pointers* m_owner             = nullptr;
    svector_pointers const* m_owner_const = nullptr;
};

/*template<class K, class T>
class svector_fix // NOLINT
{
public:
    using index_map_t    = std::unordered_map<K, size_t>;
    using const_iterator = typename index_map_t::const_iterator;

    void clear()
    {
        m_index_map.clear();
        m_vector.clear();
    }

    void push_back(T _x)
    {
        ASSERT(!m_index_map.count(_x->get_id()), math::exception, "Key already exists in svector_fix!");
        m_index_map.emplace(_x->get_id(), m_vector.size());
        m_vector.emplace_back(_x->get_id(), std::move(_x));
    }

    void emplace_back(K const& _key, T const& _x)
    {
        ASSERT(!m_index_map.count(_key), math::exception, "Key already exists in svector_fix!");
        m_index_map.emplace(_key, m_vector.size());
        m_vector.emplace_back(_key, std::move(_x));
    }

    bool contains(K const& _key) const { return m_index_map.contains(_key); }

    std::vector<std::tuple<K, T> > const& vector() const { return m_vector; }
    [[nodiscard]] size_t size() const { return m_vector.size(); }
    const_iterator find(K const& _key) const { return m_index_map.find(_key); }
    const_iterator end() const { return m_index_map.end(); }

    void null_element(K const& _key)
    {
        ASSERT(m_index_map.contains(_key), math::exception, "Key not found in svector_fix!");
        std::get<1>(m_vector.at(m_index_map.at(_key))) = nullptr;
    }

    void erase(K const& _key, bool const _sanity_check = true)
    {
        ASSERT(m_index_map.contains(_key), math::exception, "Key not found in svector_fix!");
        if (_sanity_check)
            ASSERT(std::get<1>(m_vector.at(m_index_map.at(_key)))->get_id() == _key, math::exception,
               "Key does not match in sanity check!");
        _remove(_key);
    }

    void erase(const_iterator _it)
    {
        _remove(_it->first);
    }

private:
    void _remove(K const& _key)
    {
        size_t const delete_pos = m_index_map.at(_key);
        size_t last_ind         = m_vector.size() - 1;

        m_index_map.erase(_key);
        if (last_ind != delete_pos)
        {
            auto const& last_key     = std::get<0>(m_vector.at(last_ind));
            m_index_map.at(last_key) = delete_pos;
            std::swap(m_vector.at(last_ind), m_vector.at(delete_pos));
        }

        m_vector.pop_back();

#if SOL_ENABLE_ASSERTS
        for (size_t i = 0; i < m_vector.size(); ++i)
        {
            auto const& key = std::get<0>(m_vector.at(i));
            auto& el        = std::get<1>(m_vector.at(i));
            if (el != nullptr)
                ASSERT(key == el->getKey(), math::exception, "Element key does not match!");
            ASSERT(m_index_map.at(key) == i, math::exception, "Index map does not match!");
        }
#endif
    }

private:
    std::vector<std::tuple<K, T> > m_vector;
    std::unordered_map<K, size_t> m_index_map;
};

template<class K, class T>
class svector // NOLINT
{
public:
    using index_map_t    = std::unordered_map<K, size_t>;
    using const_iterator = typename index_map_t::const_iterator;

    T& at(size_t _i) { return m_vector.at(_i); }
    T& at(K const& _key) { return m_vector.at(m_index_map.at(_key)); }
    [[nodiscard]] size_t size() const { return m_vector.size(); }
    //bool count(K const& _key) const { return m_index_map.count(_key); }
    const_iterator find(K const& _key) const { return m_index_map.find(_key); }
    const_iterator end() const { return m_index_map.end(); }

    std::vector<T>& vector() { return m_vector; }
    std::vector<T> const& vector() const { return m_vector; }
    std::unordered_map<K, size_t>& map() { return m_index_map; }

    void push_back(T&& _x)
    {
        // TODO error on double push back?
        if (find(_x->getKey()) != end())
        {
            throw std::exception();
            return;
        }
        m_index_map.emplace(_x->getKey(), m_vector.size());
        m_vector.push_back(std::move(_x));
    }

    void push_back(T const& _x)
    {
        // TODO error on double push back?
        if (find(_x->getKey()) != end())
        {
            throw std::exception();
            return;
        }
        m_index_map.emplace(_x->getKey(), m_vector.size());
        m_vector.push_back(_x);
    }

    void erase(const_iterator _it)
    {
        size_t const index = _it->second;
        m_index_map.erase(_it);
        remove(index); // You must define this function to erase from m_vector and fix indices
    }

    // TODO still it manages to remove non existent element somehow, called twice?
    void erase(T const& _x) { erase(_x->getKey()); }

    void erase(K const& _key)
    {
        auto index = m_index_map.at(_key);
        if (m_vector.at(index)->getKey() != _key)
            throw std::exception();
        remove(m_index_map.at(_key));
    }

protected:
    void remove(size_t _pos)
    {
        size_t const last_ind  = m_vector.size() - 1;
        auto const& delete_key = m_vector.at(_pos)->getKey();
        m_index_map.erase(delete_key);

        if (last_ind != _pos)
        {
            auto const& last_key     = m_vector.at(last_ind)->getKey();
            m_index_map.at(last_key) = _pos;
            std::swap(m_vector.at(last_ind), m_vector.at(_pos));
        }

        m_vector.resize(last_ind);
    }

private:
    std::vector<T> m_vector;
    std::unordered_map<K, size_t> m_index_map;
};
*/

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
