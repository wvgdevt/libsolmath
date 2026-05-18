/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once

namespace sol::math {
template<std::ranges::random_access_range R>
decltype(auto) random_element(R const& _r)
{
    VERIFY(!std::ranges::empty(_r), math::exception, "empty range!");
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<std::ranges::range_size_t<R> > dist(0, std::ranges::size(_r) - 1);
    return _r[dist(gen)];
}

template<std::ranges::sized_range R> requires (!std::ranges::random_access_range<R>)
decltype(auto) random_element(R const& _r)
{
    VERIFY(!std::ranges::empty(_r), math::exception, "empty range!");
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<std::ranges::range_size_t<R> > dist(0, std::ranges::size(_r) - 1);
    auto it = std::ranges::begin(_r);
    std::ranges::advance(it, dist(gen));
    return *it;
}

template<std::ranges::range R>
decltype(auto) random_element(R&&) = delete;

template<std::ranges::sized_range R>
auto random_element_value(R&& _r)
{
    VERIFY(!std::ranges::empty(_r), math::exception, "empty range!");
    static thread_local std::mt19937 gen{std::random_device{}()};
    using Range = std::remove_cvref_t<R>;
    std::uniform_int_distribution<std::ranges::range_size_t<Range> > dist(0, std::ranges::size(_r) - 1);

    auto it = std::ranges::begin(_r);
    std::ranges::advance(it, dist(gen));

    return std::ranges::range_value_t<Range>(*it);
}
}
