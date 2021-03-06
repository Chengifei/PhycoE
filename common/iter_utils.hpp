/* Copyright 2017-2018 by Yifei Zheng
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This header defines wrappers for iterators that mimic Python style iterators.
 */

#ifndef ITER_UTILS_HPP
#define ITER_UTILS_HPP
#include <memory>
#include <vector>
#include <utility>
#include <type_traits>

namespace iter_utils {

template <class T>
struct non_trivial_end_iter {
    T& begin() {
        return *static_cast<T*>(this);
    }
    T& end() {
        return *static_cast<T*>(this);
    }
    bool operator!=(const T&) {
        return !static_cast<T*>(this)->exhausted();
    }
    decltype(auto) operator++() {
        return static_cast<T*>(this)->operator++();
    }
    decltype(auto) operator*() {
        return **static_cast<T*>(this);
    }
    decltype(auto) operator->() {
        return &operator*();
    }
};

template <typename T>
struct array_view {
    T* _begin;
    T* _end;
    T& operator[](int idx) const {
        return *(_begin + idx);
    }
    std::size_t size() const {
        return _end - _begin;
    }
    T* begin() const {
        return _begin;
    }
    T* end() const {
        return _end;
    }
};

}

template <typename T>
class range : public iter_utils::non_trivial_end_iter<range<T>> {
    T current;
    T terminal;
public:
    typedef T difference_type;
    typedef T value_type;
    typedef void pointer;
    typedef T reference;
    typedef std::is_integral<T> rand_acc;
    typedef std::conditional<rand_acc::value, 
        std::random_access_iterator_tag,
        std::bidirectional_iterator_tag> iterator_category;
    range(T begin, T end) : current(begin), terminal(end) {}
    void operator++() {
        ++current;
    }
    void operator--() {
        --current;
    }
    typename std::enable_if<rand_acc::value, range&>::type operator+=(T t) {
        current += t;
        return *this;
    }
    typename std::enable_if<rand_acc::value, range&>::type operator-=(T t) {
        current -= t;
        return *this;
    }
    // TODO: Finish the random access iterator
    T operator*() {
        return current;
    }
    bool exhausted() {
        return current == terminal;
    }
};

template <class T>
struct combination : iter_utils::non_trivial_end_iter<combination<T>> {
    typedef typename std::iterator_traits<T>::value_type value_type;
    const std::size_t num_in_pool;
    std::unique_ptr<value_type[]> pool;
    std::unique_ptr<std::size_t[]> indices;
    /* const */ std::size_t* indices_end;
    std::unique_ptr<value_type[]> result;
    std::size_t* cursor;
    combination(T begin, T end, std::size_t sz)
            : num_in_pool(std::distance(begin, end)) {
        result = std::make_unique<value_type[]>(sz);
        indices = std::make_unique<std::size_t[]>(sz);
        for (auto i : range<std::size_t>(0, sz)) {
            indices[i] = i;
            result[i] = std::move(*(begin++));
        }
        indices_end = indices.get() + sz;
        cursor = indices_end - 1;
        pool = std::make_unique<value_type[]>(num_in_pool);
        for (auto i : range<std::size_t>(sz, num_in_pool))
            pool[i] = std::move(*(begin++));
    }
    combination& operator++() {
        while (*cursor + (indices_end - cursor) == num_in_pool) {
            --cursor;
            if (cursor - indices.get() == -1) {
                result = nullptr;
                return *this;
            }
        }
        for (std::size_t* ptr = cursor; ptr != indices_end; ++ptr)
            pool[*ptr] = std::move(result[ptr - indices.get()]);
        ++*cursor;
        result[cursor - indices.get()] = std::move(pool[*cursor]);
        for (++cursor; cursor != indices_end; ++cursor) {
            *cursor = *(cursor - 1) + 1;
            result[cursor - indices.get()] = std::move(pool[*cursor]);
        }
        --cursor;
        return *this;
    }
    bool exhausted() {
        return !static_cast<bool>(result);
    }
    const value_type* operator*() {
        return result.get();
    }
};

struct powerset : iter_utils::non_trivial_end_iter<powerset> {
    int idx = 0;
    const std::size_t pool_sz, sz; // FIXME, order
    std::vector<std::size_t> indices;
    powerset(std::size_t pool_sz, std::size_t sz)
        : pool_sz(pool_sz), sz(sz) {
        indices.reserve(sz);
        indices.push_back(0);
        resize();
    }
    powerset& operator++() {
        while (indices[idx] + (indices.size() - idx) == pool_sz) {
            --idx;
            if (idx == -1) {
                resize();
                return *this;
            }
        }
        ++indices[idx];
        for (unsigned _idx = idx + 1; _idx != indices.size(); ++_idx) {
            indices[_idx] = indices[_idx - 1] + 1;
        }
        idx = indices.size() - 1;
        return *this;
    }
    bool exhausted() {
        return indices.empty();
    }
    const std::vector<std::size_t>& operator*() {
        return indices;
    }
    const std::vector<std::size_t>* operator->() {
        return &indices;
    }
private:
    void resize() {
        if (indices.size() != sz) {
            idx = indices.size();
            for (std::size_t _idx = 0; _idx != indices.size(); ++_idx) {
                indices[_idx] = _idx;
            }
            indices.push_back(indices.size());
        }
        else {
            indices.clear();
        }
    }
};

template <class TIT, class UIT>
struct product : iter_utils::non_trivial_end_iter<product<TIT, UIT>> {
    typedef typename TIT::value_type T;
    typedef typename UIT::value_type U;
    TIT begin1, it1, end1;
    UIT begin2, it2, end2;
    product(TIT begin1, TIT end1, UIT begin2, UIT end2)
        : begin1(begin1), it1(begin1), end1(end1), begin2(begin2), it2(begin2), end2(end2) {}
    product& operator++() {
        if (it1 != end1)
             ++it1;
        if (it1 == end1) {
            ++it2;
            it1 = begin1;
        }
        return *this;
    }
    std::pair<T, U> operator*() {
        return std::make_pair(*it1, *it2);
    }
    bool exhausted() const {
        return it2 == end2;
    }
};
#endif
