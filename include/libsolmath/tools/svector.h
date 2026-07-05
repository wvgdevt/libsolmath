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
#include <cassert>
#include <unordered_set>

#include "exception.h"
#include "logger.h"

namespace sol::math {
enum class erase_mode { // NOLINT
    strict,             // NOLINT
    allow_pending_add   // NOLINT
};

// Helper trait that extracts the "object type" stored behind a value.
// By default, the type is assumed to already be the object type.
// Example: pointee_type<Foo>::type == Foo
template<class P>
struct pointee_type {
    using type = P;
};

// Specialization for raw pointers.
// Removes one pointer layer and returns the pointed-to object type.
// Example: pointee_type<Foo*>::type == Foo
template<class U>
struct pointee_type<U*> {
    using type = U;
};

// Specialization for std::unique_ptr.
// Extracts the managed object type from unique_ptr.
// The deleter type is ignored because it does not affect the object type.
// Example: pointee_type<std::unique_ptr<Foo>>::type == Foo
template<class U, class Deleter>
struct pointee_type<std::unique_ptr<U, Deleter> > {
    using type = U;
};

// Fast iteration class with key access, for pointers
template<class K, class T>
class svector {                                                               // NOLINT
    using object_type = typename pointee_type<std::remove_cvref_t<T> >::type; // NOLINT

    // Returns a raw pointer to the stored object, regardless of how T stores it.
    // Supported storage forms:
    //   - raw pointer:______T = Object*
    //   - unique_ptr-like:__T = std::unique_ptr<Object>
    //   - direct object:____T = Object
    // This is used internally to treat all supported storage types uniformly.
    [[nodiscard]] static object_type* _ptr(T& _x) noexcept
    {
        // Case 1: T itself is a raw pointer.
        // Example: T = Object*
        // _x already points to the object, so return it directly.
        if constexpr (std::is_pointer_v<std::remove_cvref_t<T> >) { return _x; }
        // Case 2: T is a pointer-like owner/wrapper with get().
        // Example: T = std::unique_ptr<Object>
        // get() returns the managed raw pointer.
        else if constexpr (requires { { _x.get() } -> std::convertible_to<object_type*>; })
        {
            return _x.get();
        }
        // Case 3: T is the object itself.
        // Example: T = Object
        // Return the address of the stored object.
        else
            return &_x;
    }

    // Same as above, but object_type const* version for T const&
    [[nodiscard]] static object_type const* _ptr(T const& _x) noexcept
    {
        if constexpr (std::is_pointer_v<std::remove_cvref_t<T> >) { return _x; }
        else if constexpr (requires { { _x.get() } -> std::convertible_to<object_type const*>; }) { return _x.get(); }
        else return &_x;
    }

public:
    using index_map_t    = std::unordered_map<K, size_t>;        // NOLINT
    using const_iterator = typename index_map_t::const_iterator; // NOLINT

    // Removes all stored objects and all pending buffered modifications.
    //   This operation is only allowed when the container is not being iterated.
    //   Clearing during iteration would invalidate the active iteration state
    //   and make buffered add/remove logic ambiguous.
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

    // TODO check buffers?
    bool empty() const
    {
        return m_vector.empty();
    }

    std::vector<T> const& vector_unsafe() const { return m_vector; }
    [[nodiscard]] size_t size() const { return m_vector.size(); }

    // Returns true if the key is currently considered present in the container.
    //   This function respects pending buffered modifications:
    //    - if the key is scheduled for removal, it is treated as absent;
    //    - if the key already exists in the main index map, it is present;
    //    - if the key is scheduled for insertion, it is also treated as present.
    //   This means contains() reflects the logical state of the container,
    //   not only the already-applied physical state of m_vector/m_index_map.
    [[nodiscard]] bool contains(K const _key) const
    {
        if (m_set_remove_buffer_keys.contains(_key))
            return false;

        if (m_index_map.contains(_key))
            return true;

        return m_set_add_buffer_keys.contains(_key);
    }

    // Finds an object by key and returns a mutable raw pointer to it.
    //   Returns nullptr if the key is not currently considered present.
    //   Like contains(), this function respects buffered modifications:
    //   objects scheduled for removal are treated as absent, and objects scheduled
    //   for insertion may be found before the buffer is physically applied.
    [[nodiscard]] object_type* try_get(K const _key) { return _try_get_impl(*this, _key); }
    [[nodiscard]] object_type const* try_get(K const _key) const { return _try_get_impl(*this, _key); }

    // Finds the first object matching the given predicate and returns a mutable pointer.
    //   The predicate is called with: object_type const&
    //   even for the mutable overload. This means the predicate may inspect objects,
    //   but it should not modify them while searching.
    // Returns nullptr if no object matches.
    //   Like try_get(), the implementation may respect the logical container state,
    //   depending on _find_if_impl(): committed objects scheduled for removal should be
    //   skipped, and pending added objects may also be searched.
    template<class Predicate>
        requires std::predicate<Predicate&, object_type const&>
    [[nodiscard]] object_type* find_if(Predicate&& _condition)
    {
        return _find_if_impl(*this, std::forward<Predicate>(_condition));
    }

    // Same as above, but const version
    template<class Predicate>
        requires std::predicate<Predicate&, object_type const&>
    [[nodiscard]] object_type const* find_if(Predicate&& _condition) const
    {
        return _find_if_impl(*this, std::forward<Predicate>(_condition));
    }

    // View the type used for mutable iteration over the container.
    //   mutable_iteration() returns this view. The view is responsible for registering
    //   that iteration is active and for applying buffered modifications when iteration finishes.
    class iteration_view;

    // Starts mutable iteration over the container.
    //   During this iteration, container modifications are allowed, but they should
    //   not be applied directly to m_vector while the loop is active. Instead,
    //   additions/removals are expected to go through the buffer system.
    // Example:
    //      for (auto& object: objects.mutable_iteration()) { ... }
    //   Returns an iteration_view object whose lifetime represents the active iteration scope.
    [[nodiscard]] iteration_view mutable_iteration() { return iteration_view(*this); }

    // View the type used for const iteration over the container.
    //  const_iteration() returns this view. It allows read-only access to the stored
    //  objects while keeping the container protected from direct modification.
    class const_iteration_view;

    // Starts const iteration over the container.
    //   The returned view provides read-only access to the objects. This is intended
    //   for loops that only inspect objects and do not mutate container structure.
    // Example:
    //     for (auto const& object: objects.const_iteration()) { ... }
    [[nodiscard]] const_iteration_view const_iteration() const { return const_iteration_view(*this); }

    // Returns true if no stored element satisfies the given predicate.
    // This is a thin wrapper around std::ranges::none_of().
    // Important:
    //   The predicate is applied directly to elements stored in m_vector.
    // Therefore, depending on T, the predicate receives:
    //     T = Object                      -> Object&
    //     T = Object*                     -> Object*&
    //     T = std::unique_ptr<Object>     -> std::unique_ptr<Object>&
    // So this function currently works on the storage type T, not necessarily on object_type const&.
    template<typename Predicate>
    [[nodiscard]] bool none(Predicate&& _predicate) const
    {
        return std::ranges::none_of(m_vector, std::forward<Predicate>(_predicate));
    }

    // Adds a new element to the container.
    //   The input value must be constructible into the container storage type T.
    // Examples:
    //     T = Object                      -> emplace(Object{...})
    //     T = Object*                     -> emplace(object_ptr)
    //     T = std::unique_ptr<Object>     -> emplace(std::move(object_ptr))
    // The element key is taken from: object.get_id()
    // This means every stored object must provide get_id().
    template<class U>
        requires std::constructible_from<T, U&&>
    void emplace(U&& _x)
    {
        auto* ptr_x = _ptr(_x);
        DEBUG_ASSERT(ptr_x != nullptr, math::exception, "emplace null pointer in svector!");
        auto const key = ptr_x->get_id();
        if (!m_iteration_modes.empty())
        {
            VERIFY(_modification_allowed(), math::exception, "emplace while const_iteration!");
            VERIFY(!m_index_map.contains(key), math::exception, "Key already exists!");
            _emplace_while_iteration(key, std::forward<U>(_x));
            return;
        }

        DEBUG_ASSERT(key == ptr_x->get_id(), math::exception, "key does not match element id in svector!");
        _emplace_exception_free(key, std::forward<U>(_x));
    }

    // Removes an element from the container by key.
    //   If no iteration is active, the element is removed immediately from the committed dense storage.
    //   If iteration is active, the removal is buffered and applied after iteration finishes.
    //   This avoids invalidating the active iterator/view.
    void erase(K const _key, erase_mode const _mode = erase_mode::strict)
    {
        if (!m_iteration_modes.empty())
        {
            VERIFY(_modification_allowed(), math::exception, "erase while const_iteration!");
            _erase_while_iteration(_key, _mode);
            return;
        }

        auto const it_index_map = m_index_map.find(_key);
        VERIFY(it_index_map != m_index_map.end(), math::exception, "_key not found!");
        _remove(it_index_map);
    }

    // Removes an element by key if it is currently present.
    //   Unlike erase(), this function does not treat a missing key as an error.
    //   If the key is not found, the function simply returns.
    //   If no iteration is active, the element is removed immediately.
    //   If iteration is active, removal is buffered and applied after iteration finishes.
    void erase_if_present(K const _key)
    {
        if (!m_iteration_modes.empty())
        {
            if (!m_index_map.contains(_key) && !m_set_add_buffer_keys.contains(_key))
                return;

            VERIFY(_modification_allowed(), math::exception, "erase_if_present while const_iteration!");

            _erase_while_iteration(_key, erase_mode::strict);
            return;
        }

        auto const it_index_map = m_index_map.find(_key);
        if (it_index_map != m_index_map.end())
            _remove(it_index_map);
    }

    // Removes an element by key and returns the removed storage value.
    //   This function extracts the actual stored T value from the container.
    // Examples:
    //     T = Object*_________________-> returns removed Object*
    //     T = std::unique_ptr<Object> -> returns removed std::unique_ptr<Object>
    //     T = Object__________________-> returns removed Object
    // If the key is not found, returns nullptr.
    // Important:
    //   This operation is not allowed during iteration because it removes the
    //   element immediately and returns ownership/value to the caller. That would
    //   invalidate the active iteration state and cannot be represented as a buffered operation.
    T try_remove_extract(K const _key)
    {
        VERIFY(m_iteration_modes.empty(), math::exception, "remove_extract while iteration!");
        auto const it_index_map = m_index_map.find(_key);
        if (it_index_map != m_index_map.end())
            return _remove(it_index_map);
        return nullptr;
    }

private:
    template<class Self>
    [[nodiscard]] static auto _try_get_impl(Self& _self, K const _key)
    {
        using self_t   = std::remove_reference_t<Self>; // NOLINT
        using return_t = std::conditional_t<            // NOLINT
            std::is_const_v<self_t>,
            object_type const*,
            object_type*>;

        if (_self.m_set_remove_buffer_keys.contains(_key))
            return static_cast<return_t>(nullptr);

        auto const it = _self.m_index_map.find(_key);
        if (it != _self.m_index_map.end())
        {
            DEBUG_ASSERT(it->second < _self.m_vector.size(), math::exception, "Out of bounds in try_get!");
            return static_cast<return_t>(_ptr(_self.m_vector[it->second]));
        }

        auto buffer_it = _self._find_add_buffer(_key);
        if (buffer_it != _self.m_vector_add_buffer.end())
            return static_cast<return_t>(_ptr(buffer_it->second));

        return static_cast<return_t>(nullptr);
    }

    template<class Self, class Predicate>
    [[nodiscard]] static auto _find_if_impl(Self& _self, Predicate&& _condition)
    {
        using self_t   = std::remove_reference_t<Self>; // NOLINT
        using return_t = std::conditional_t<            // NOLINT
            std::is_const_v<self_t>,
            object_type const*,
            object_type*>;

        for (auto& el: _self.m_vector)
        {
            auto* object = _ptr(el);
            DEBUG_ASSERT(object != nullptr, math::exception, "Null element in find_if!");
            if (std::invoke(_condition, std::as_const(*object)))
                return static_cast<return_t>(object);
        }

        for (auto& [key, el]: _self.m_vector_add_buffer)
        {
            auto* object = _ptr(el);
            DEBUG_ASSERT(object != nullptr, math::exception, "Null add-buffer element in find_if!");
            if (std::invoke(_condition, std::as_const(*object)))
                return static_cast<return_t>(object);
        }
        return static_cast<return_t>(nullptr);
    }

    // Inserts a new element directly into committed storage.
    //   This function is used when no iteration is active, so the element can be
    //   inserted immediately into m_vector instead of going through the add-buffer.
    // Preconditions:
    //   - _key must not yet exist in m_index_map.
    //   - _x must be constructible into T.
    //   - no active iteration should be modifying/reading m_vector structurally.
    // The key is inserted into m_index_map with the index where the new element will be stored in m_vector.
    template<class U>
        requires std::constructible_from<T, U&&>
    void _emplace_exception_free(K const _key, U&& _x)
    {
        VERIFY(m_index_map.emplace(_key, m_vector.size()).second, math::exception, "Key already exists!");
        m_vector.emplace_back(std::forward<U>(_x));
    }

    // Schedules a new element for insertion after the current iteration finishes.
    //   This function is used only while an iteration view is active. It does not
    //   modify the committed dense storage immediately because doing so could invalidate the active iteration.
    // Instead, it records:
    //   - the key in m_set_add_buffer_keys for fast duplicate checks;
    //   - the actual element in m_vector_add_buffer for later insertion.
    // Important design rule:
    //   Pending operations are not cancellable.
    // Therefore, adding a key already scheduled for removal is invalid.
    // Likewise, adding the same key twice before buffers are applied is invalid.
    template<class U>
        requires std::constructible_from<T, U&&>
    void _emplace_while_iteration(K const _key, U&& _x)
    {
        VERIFY(!m_set_remove_buffer_keys.contains(_key), math::exception,
               "key already scheduled for erase in svector!");
        VERIFY(m_set_add_buffer_keys.emplace(_key).second, math::exception,
               "key already scheduled for emplace in svector!");
        m_vector_add_buffer.emplace_back(_key, std::forward<U>(_x));
    }

    // Schedules an existing committed element for removal after iteration finishes.
    //   This function is used only while an iteration view is active. It does not
    //   physically erase the vector slot immediately because that would invalidate the active iteration.
    // Important design rule:
    //   Pending operations are not cancellable.
    // Therefore:
    //   - a key scheduled for adding cannot be erased;
    //   - a key scheduled for erasing cannot be erased again.
    void _erase_while_iteration(K const _key, erase_mode const _mode)
    {
        auto const add_buffer_it = m_set_add_buffer_keys.find(_key);
        if (_mode == erase_mode::allow_pending_add && add_buffer_it != m_set_add_buffer_keys.end())
        {
            auto it = _find_add_buffer(_key);
            VERIFY(it != m_vector_add_buffer.end(), math::exception, "add-buffer key exists but object not found");
            m_vector_add_buffer.erase(it);
            m_set_add_buffer_keys.erase(add_buffer_it);
            return;
        }

        VERIFY(add_buffer_it == m_set_add_buffer_keys.end(), math::exception,
               "key already scheduled for emplace in svector!");
        VERIFY(m_set_remove_buffer_keys.emplace(_key).second, math::exception,
               "key already scheduled for erase in svector!");
        if constexpr (std::is_pointer_v<T>)
            _null_element(_key);
    }

    // Replaces the stored element for the given key with nullptr.
    //   This does not remove the key from m_index_map and does not erase the slot
    //   from m_vector. It only clears the stored value at the existing index.
    // This is useful only for pointer-like storage:
    //     T = Object*
    //     T = std::unique_ptr<Object>
    // It is not valid for direct-object storage:
    //     T = Object
    // because Object usually cannot be assigned from nullptr.
    void _null_element(K const _key)
    {
        auto const it = m_index_map.find(_key);
        VERIFY(it != m_index_map.end(), math::exception, "Key not found in svector!");
        DEBUG_ASSERT(it->second < m_vector.size(), math::exception, "Out of bounds in _null_element!");
        m_vector[it->second] = nullptr;
    }

    // Applies all buffered modifications after iteration has finished.
    //   Buffered removals are processed first, then buffered additions are committed.
    //   This function is expected to be called only when the no iteration view is active.
    // While iteration is active, structural modifications are stored in:
    //   - m_set_remove_buffer_keys
    //   - m_vector_add_buffer
    //   - m_set_add_buffer_keys
    // After this function finishes, all buffers are cleared and the committed storage reflects the pending changes.
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

    // Removes the element referenced by an index-map iterator and returns the extracted stored value.
    //   The container keeps m_vector dense by moving the last valid element into the
    //   erased slot, then popping the last slot.
    // This function also handles slots that were nulled earlier by
    //   _erase_while_iteration(). In that case, the object may already be destroyed,
    //   but its vector slot and index-map entry may still exist until buffer processing finishes.
    T _remove(const_iterator _it_index_map)
    {
        size_t const delete_pos = _it_index_map->second;

        // Only allow null_pointer when T = type*
        if constexpr (std::is_pointer_v<T>)
        {
            while (!m_vector.empty() && _ptr(m_vector.back()) == nullptr)
                m_vector.pop_back();
        }

        m_index_map.erase(_it_index_map);

        // Only allow null_pointer when T = type*
        if constexpr (std::is_pointer_v<T>)
        {
            // The requested element was null and was removed while trimming the tail.
            if (delete_pos >= m_vector.size())
                return nullptr;
        }

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

    // Returns true if structural modification is currently allowed.
    //   The container may have one or more active iteration views. Each view records
    //   whether modification is allowed during its lifetime.
    // Modification is allowed only if none of the active iteration modes is:
    //     container_modification::not_allowed
    // This means:
    //   - mutable_iteration() can allow buffered emplace/erase;
    //   - const_iteration() can block structural modification.
    // If m_iteration_modes is empty, std::ranges::none_of() returns true.
    // Therefore, modification is allowed when no iteration is active.
    [[nodiscard]] bool _modification_allowed() const
    {
        return std::ranges::none_of(m_iteration_modes, [](container_modification const _mode) {
            return _mode == container_modification::not_allowed;
        });
    }

    // Finds a pending-add entry by key.
    [[nodiscard]] auto _find_add_buffer(K const _key)
    {
        return std::ranges::find_if(m_vector_add_buffer, [_key](auto const& _pair) {
            return _key == _pair.first;
        });
    }

    // Finds a pending-add entry by key const.
    [[nodiscard]] auto _find_add_buffer(K const _key) const
    {
        return std::ranges::find_if(m_vector_add_buffer, [_key](auto const& _pair) {
            return _key == _pair.first;
        });
    }

private:
    enum class container_modification { // NOLINT
        allowed,                        // NOLINT
        not_allowed,                    // NOLINT
    };

    mutable std::vector<container_modification> m_iteration_modes;
    std::vector<T> m_vector;
    std::vector<std::pair<K, T> > m_vector_add_buffer;
    std::unordered_set<K> m_set_add_buffer_keys;
    std::unordered_set<K> m_set_remove_buffer_keys;
    std::unordered_map<K, size_t> m_index_map;
};

template<class K, class T>
class svector<K, T>::iteration_view {
public:
    // Creates a mutable iteration view for the given svector.
    //   The lifetime of this view represents an active iteration scope.
    //   While this view exists, the owner knows that iteration is in progress.
    //
    // Structural modifications such as emplace()/erase() must not modify
    //   m_vector directly during this time. They should be redirected into the
    //   pending add/remove buffers instead.
    explicit iteration_view(svector& _owner)
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
            m_owner->_process_buffer();
    }

    auto begin() const noexcept { return m_owner->m_vector.begin(); }
    auto end() const noexcept { return m_owner->m_vector.end(); }

private:
    size_t m_owner_size_start;
    svector* m_owner = nullptr;
};

template<class K, class T>
class svector<K, T>::const_iteration_view {
public:
    // Creates an immutable iteration view for the given svector.
    //   The lifetime of this view represents an active iteration scope.
    //   While this view exists, the owner knows that iteration is in progress.
    //
    // Structural modifications such as emplace()/erase() must not be called during this iteration
    explicit const_iteration_view(svector const& _owner)
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
    svector const* m_owner_const = nullptr;
};
}
