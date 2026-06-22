/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#include "tests.h"
#include "libsolmath/tools/svector_pointers.h"

#define PRINT_FUNC_NAME() std::cout << __PRETTY_FUNCTION__ << std::endl;

namespace sol::math::tests::svector {
struct TestObject {
    explicit TestObject(int const _id)
        : id(_id)
    {}

    [[nodiscard]] int get_id() const noexcept
    {
        return id;
    }

    int id;
    int updates = 0;
};

using TestVector = sol::math::svector_pointers<int, TestObject>;

void test_emplace_find_iteration_order()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};
    TestObject c{3};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);
    v.emplace(&c);

    assert(v.size() == 3);

    assert(v.try_get(1) == &a);
    assert(v.try_get(2) == &b);
    assert(v.try_get(3) == &c);

    std::vector<int> ids;
    for (auto* el: v.const_iteration())
    {
        assert(el != nullptr);
        ids.push_back(el->get_id());
    }

    assert((ids == std::vector<int>{1, 2, 3}));
}

void test_remove_non_iterating()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};
    TestObject c{3};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);
    v.emplace(&c);

    v.erase(2);

    assert(v.size() == 2);
    assert(v.try_get(1) == &a);
    assert(v.try_get(3) == &c);

    // Depends on your API:
    assert(!v.contains(2));
}

void test_remove_uses_swap_remove_and_updates_index()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};
    TestObject c{3};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);
    v.emplace(&c);

    // Remove middle element.
    // If implementation uses swap-remove, object 3 may move into index of object 2.
    v.erase(2);

    assert(v.size() == 2);

    // Critical part: key -> index map must still be correct.
    assert(v.try_get(1) == &a);
    assert(v.try_get(3) == &c);
    assert(!v.contains(2));
}

void test_deferred_remove_during_mutable_iteration()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};
    TestObject c{3};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);
    v.emplace(&c);

    for (auto* el: v.mutable_iteration())
    {
        assert(el != nullptr);

        if (el->get_id() == 2)
            v.erase(2);

        el->updates++;
    }

    // After iteration, deferred remove should be flushed.
    assert(v.size() == 2);
    assert(v.try_get(1) == &a);
    assert(v.try_get(3) == &c);
    assert(!v.contains(2));

    // Object 2 was still visited in this policy.
    // If your desired policy is "deleted object must not update",
    // then this expected value should be different.
    assert(b.updates == 1);
}

void test_deferred_add_during_mutable_iteration()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};
    TestObject c{3};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);

    std::vector<int> visited;

    for (auto* el: v.mutable_iteration())
    {
        assert(el != nullptr);
        visited.push_back(el->get_id());

        if (el->get_id() == 1)
        {
            v.emplace(&c);
        }
    }

    // New object should not be visited during the same iteration.
    assert((visited == std::vector<int>{1, 2}));

    // But it should exist after iteration flush.
    assert(v.size() == 3);
    assert(v.try_get(3) == &c);
}

void test_duplicate_key_throws()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};

    TestVector v;

    v.emplace(&a);

    bool thrown = false;

    try
    {
        v.emplace(&a);
    } catch (...)
    {
        thrown = true;
    }

    assert(thrown);
    v.clear();

    thrown = false;
    try
    {
        v.emplace(&a);
        for (auto* el: v.mutable_iteration())
        {
            (void) el;
            v.emplace(&b);
            v.emplace(&b);
        }
    } catch (...)
    {
        thrown = true;
    }
    assert(thrown);
}

void test_remove_then_emplace_during_iteration_fails()
{
    PRINT_FUNC_NAME();
    TestVector v;

    TestObject a{1};
    TestObject b{2};

    v.emplace(&a);

    bool thrown = false;
    try
    {
        for (auto* object: v.mutable_iteration())
        {
            (void) object;
            v.erase(a.get_id());
            v.emplace(&a);
        }
    } catch (...)
    {
        thrown = true;
    }
    assert(thrown);
}

void test_emplace_then_remove_during_iteration_fine()
{
    PRINT_FUNC_NAME();
    TestVector v;

    TestObject a{1};
    TestObject b{2};

    v.emplace(&b);

    bool thrown = false;
    try
    {
        for (auto* object: v.mutable_iteration())
        {
            (void) object;
            v.emplace(&a);
            v.erase(a.get_id());
        }
    } catch (...)
    {
        thrown = true;
    }
    assert(!thrown);
}

void test_emplace_then_remove_then_emplace_during_iteration_is_fine()
{
    PRINT_FUNC_NAME();
    TestVector v;

    TestObject a{1};
    TestObject b{2};

    v.emplace(&b);

    bool thrown = false;
    try
    {
        for (auto* object: v.mutable_iteration())
        {
            (void) object;
            v.emplace(&a);
            v.erase(a.get_id());
            v.emplace(&a);
        }
    } catch (...)
    {
        thrown = true;
    }
    assert(!thrown);
}

void test_remove_then_emplace_without_iteration_is_fine()
{
    PRINT_FUNC_NAME();
    TestVector v;

    TestObject a{1};
    TestObject b{2};

    v.emplace(&a);

    bool thrown = false;
    try
    {
        v.erase(a.get_id());
        v.emplace(&a);
    } catch (...)
    {
        thrown = true;
    }
    assert(!thrown);
}

void test_const_iteration_disallows_modification()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);

    bool thrown = false;

    try
    {
        for (auto* el: v.const_iteration())
        {
            assert(el != nullptr);

            if (el->get_id() == 1)
            {
                v.erase(1);
            }
        }
    } catch (...)
    {
        thrown = true;
    }

    assert(thrown);
}

void test_nested_iteration_keeps_strictest_mode()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject b{2};
    TestObject c{3};

    TestVector v;

    v.emplace(&a);
    v.emplace(&b);
    v.emplace(&c);

    bool thrown = false;

    try
    {
        for (auto* outer: v.const_iteration())
        {
            assert(outer != nullptr);

            for (auto* inner: v.mutable_iteration())
            {
                assert(inner != nullptr);

                // This should still be forbidden because outer iteration is not_allowed.
                v.erase(inner->get_id());
            }
        }
    } catch (...)
    {
        thrown = true;
    }

    assert(thrown);

    // Nothing should be corrupted.
    assert(v.size() == 3);
    assert(v.try_get(1) == &a);
    assert(v.try_get(2) == &b);
    assert(v.try_get(3) == &c);
}

void test_erase_same_key_twice_is_safe_or_throws_consistently()
{
    PRINT_FUNC_NAME();
    TestObject a{1};

    TestVector v;

    v.emplace(&a);
    v.erase(1);

    assert(v.size() == 0);
    assert(!v.contains(1));

    bool thrown = false;

    try
    {
        v.erase(1);
    } catch (...)
    {
        thrown = true;
    }

    // Choose ONE policy:
    //
    // Policy A: erase missing key throws.
    assert(thrown);
    //
    // Policy B: erase missing key is ignored.
    // assert(!thrown);
}

void test_erase_existing_then_add_same_key_during_iteration_throws()
{
    PRINT_FUNC_NAME();
    TestObject a{1};
    TestObject replacement{1};

    TestVector v;
    v.emplace(&a);

    bool thrown = false;

    try
    {
        for (auto* el: v.mutable_iteration())
        {
            assert(el != nullptr);

            v.erase(1);
            assert(v.try_get(1) == nullptr);

            v.emplace(&replacement);
        }
        // TODO there shall be no execption thrown during buffer processing as it is in destructor
        // throw exceptions in emplace erase while iteration
    } catch (math::exception const&)
    {
        thrown = true;
    }

    assert(thrown);
}
}

namespace sol::math::tests {
using namespace sol::math::tests::svector;

void test_svector()
{
    test_emplace_find_iteration_order();
    test_remove_non_iterating();
    test_remove_uses_swap_remove_and_updates_index();
    test_deferred_remove_during_mutable_iteration();
    test_deferred_add_during_mutable_iteration();
    test_duplicate_key_throws();
    test_const_iteration_disallows_modification();
    test_nested_iteration_keeps_strictest_mode();
    test_erase_same_key_twice_is_safe_or_throws_consistently();
    test_erase_existing_then_add_same_key_during_iteration_throws();
    test_remove_then_emplace_during_iteration_fails();
    test_remove_then_emplace_without_iteration_is_fine();
    test_emplace_then_remove_during_iteration_fine();
    test_emplace_then_remove_then_emplace_during_iteration_is_fine();
}
}

// TODO
// svector should throw on double add or double erase
// verify const iteration blocks all sub iteration modifications
// verify buffer is properly applied
// try_get shall return from the buffer, but not if object erased
