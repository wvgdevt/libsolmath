#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <cstddef>
#include <iostream>

namespace sol::math {
template<class K, class T>
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
        assert(!m_index_map.count(_x->getKey()));
        m_index_map.emplace(_x->getKey(), m_vector.size());
        m_vector.emplace_back(_x->getKey(), std::move(_x));
    }

    std::vector<std::tuple<K, T> > const& vector() const { return m_vector; }
    size_t size() const { return m_vector.size(); }
    const_iterator find(K const& _key) const { return m_index_map.find(_key); }
    const_iterator end() const { return m_index_map.end(); }

    void null_element(K const& _key)
    {
        assert(m_index_map.count(_key));
        std::get<1>(m_vector.at(m_index_map.at(_key))) = nullptr;
    }

    void erase(K const& _key, bool const _sanity_check = true)
    {
        assert(m_index_map.count(_key));
        if (_sanity_check)
            assert(std::get<1>(m_vector.at(m_index_map.at(_key)))->getKey() == _key);
        _remove(_key);
    }

    void erase(const_iterator _it)
    {
        _remove(_it->first);
    }

private:
    void _remove(K const& _key)
    {
        /*std::cerr << "REMOVE " << _key << std::endl;
        for (auto const& [id, index]: m_index_map)
            std::cerr << "id: " << id << " index: " << index << " ";
        std::cerr << std::endl;
        for (auto const& [id, el]: m_vector)
        {
            if (el == nullptr)
                std::cerr << "id: " << id << " vid: " << "null" << " ";
            else
                std::cerr << "id: " << id << " vid: " << el->getKey() << " ";
        }
        std::cerr << std::endl;
        */

        size_t const delete_pos = m_index_map.at(_key);
        size_t last_ind         = m_vector.size() - 1;

        m_index_map.erase(_key);
        if (last_ind != delete_pos)
        {
            auto const& last_key     = std::get<0>(m_vector.at(last_ind));
            m_index_map.at(last_key) = delete_pos;
            std::swap(m_vector.at(last_ind), m_vector.at(delete_pos));
        }

        //m_vector.resize(last_ind);
        m_vector.pop_back();

        // DEBUG
        /*std::cerr << "REMOVE AFTER" << std::endl;
        for (auto const& [id, index]: m_index_map)
            std::cerr << "id: " << id << " index: " << index << " ";
        std::cerr << std::endl;
        for (auto const& [id, el]: m_vector)
        {
            if (el == nullptr)
                std::cerr << "id: " << id << " vid: " << " null" << " ";
            else
                std::cerr << "id: " << id << " vid: " << el->getKey() << " ";
        }
        std::cerr << std::endl;*/

        for (size_t i = 0; i < m_vector.size(); ++i)
        {
            auto const& key = std::get<0>(m_vector.at(i));
            auto& el        = std::get<1>(m_vector.at(i));
            if (el != nullptr)
                assert(key == el->getKey());
            assert(m_index_map.at(key) == i);
        }
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
    size_t size() const { return m_vector.size(); }
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
        m_index_map.emplace(_x.getKey(), m_vector.size());
        m_vector.push_back(std::move(_x));
    }

    std::vector<T>& vector() { return m_vector; }
    void erase(K const& _key) { remove(m_index_map.at(_key)); }

protected:
    void remove(size_t _pos)
    {
        size_t const last_ind  = m_vector.size() - 1;
        auto const& delete_key = m_vector.at(_pos).getKey();
        m_index_map.erase(delete_key);

        if (last_ind != _pos)
        {
            auto const& last_key     = m_vector.at(last_ind).getKey();
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
