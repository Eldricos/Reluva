// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

// Savestates (docs/savestates.md): Reluva's own saves, for any engine. An
// engine lists its state once, in syncState functions run by a Serializer
// that either saves or loads; this file wraps the result with a header
// (which game, when, a description and a thumbnail) and keeps the slots.

#include "common/types.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace reluva::save {

class Serializer;

// Saving or loading one value. Arithmetic types, enums, strings and the
// standard containers of them are built in; an engine's own types give a
// free function syncState(Serializer&, T&) found by argument lookup.
template <class T> void sync(Serializer& s, T& v);

class Serializer {
public:
    // A saver collects bytes; a loader reads them back, in the same order.
    static Serializer saver() { return Serializer(true, {}); }
    static Serializer loader(std::vector<uint8_t> bytes) { return Serializer(false, std::move(bytes)); }

    bool saving() const { return saving_; }
    bool loading() const { return !saving_; }

    // Saves or loads `v` (any type sync() knows).
    template <class T> Serializer& operator()(T& v) {
        save::sync(*this, v);
        return *this;
    }
    // A value saved but not kept on loading (a value computed again).
    template <class T> Serializer& skip(T v) {
        save::sync(*this, v);
        return *this;
    }

    // A loader that ran past its bytes or read something impossible fails
    // (it reads zeros from then on); its first error says why.
    bool ok() const { return error_.empty(); }
    const std::string& error() const { return error_; }
    void fail(std::string why) {
        if (error_.empty())
            error_ = std::move(why);
    }
    // Whether a loader used every byte.
    bool atEnd() const { return pos_ == bytes_.size(); }

    const std::vector<uint8_t>& bytes() const { return bytes_; }
    std::vector<uint8_t> take() { return std::move(bytes_); }

    // Raw little-endian bytes of one arithmetic value (swapped on a
    // big-endian host).
    void raw(void* p, size_t n);
    // `n` bytes as they are, never swapped (a string's, a byte list's).
    void blob(void* p, size_t n);
    // A container's length: on loading, checked against what's left (each
    // element takes at least `minBytes`).
    uint32_t count(size_t n, size_t minBytes = 1);
    // Bytes a loader has yet to read.
    size_t remaining() const { return saving_ ? 0 : bytes_.size() - pos_; }

private:
    Serializer(bool saving, std::vector<uint8_t> bytes) : saving_(saving), bytes_(std::move(bytes)) {}
    bool saving_;
    std::vector<uint8_t> bytes_;
    size_t pos_ = 0;
    std::string error_;
};

namespace detail {
template <class T> struct IsVector : std::false_type {};
template <class T, class A> struct IsVector<std::vector<T, A>> : std::true_type {};
template <class T> struct IsDeque : std::false_type {};
template <class T, class A> struct IsDeque<std::deque<T, A>> : std::true_type {};
template <class T> struct IsArray : std::false_type {};
template <class T, size_t N> struct IsArray<std::array<T, N>> : std::true_type {};
template <class T> struct IsOptional : std::false_type {};
template <class T> struct IsOptional<std::optional<T>> : std::true_type {};
template <class T> struct IsPair : std::false_type {};
template <class A, class B> struct IsPair<std::pair<A, B>> : std::true_type {};
template <class T> struct IsMap : std::false_type {};
template <class K, class V, class C, class A> struct IsMap<std::map<K, V, C, A>> : std::true_type {};
template <class K, class V, class H, class E, class A>
struct IsMap<std::unordered_map<K, V, H, E, A>> : std::true_type {};
template <class T> struct IsSet : std::false_type {};
template <class K, class C, class A> struct IsSet<std::set<K, C, A>> : std::true_type {};
template <class T> struct IsUnique : std::false_type {};
template <class T> struct IsUnique<std::unique_ptr<T>> : std::true_type {};

// The fewest bytes sync() writes for a T (0 when it can't tell: an
// engine's own type), so a loader can check a count against what's left.
template <class T> constexpr size_t minBytes() {
    if constexpr (std::is_same_v<T, bool>)
        return 1;
    else if constexpr (std::is_arithmetic_v<T>)
        return sizeof(T);
    else if constexpr (std::is_enum_v<T>)
        return sizeof(std::underlying_type_t<T>);
    else if constexpr (std::is_same_v<T, std::string> || IsVector<T>::value || IsDeque<T>::value ||
                       IsMap<T>::value || IsSet<T>::value)
        return 4;
    else if constexpr (std::is_array_v<T>)
        return std::extent_v<T> * minBytes<std::remove_extent_t<T>>();
    else if constexpr (IsArray<T>::value)
        return std::tuple_size_v<T> * minBytes<typename T::value_type>();
    else if constexpr (IsOptional<T>::value || IsUnique<T>::value)
        return 1;
    else if constexpr (IsPair<T>::value)
        return minBytes<std::remove_const_t<typename T::first_type>>() + minBytes<typename T::second_type>();
    else
        return 0;
}

// Bytes copied as they are: one-byte numbers (not bool, which is checked).
template <class T>
constexpr bool kIsByte = std::is_arithmetic_v<T> && sizeof(T) == 1 && !std::is_same_v<T, bool>;
} // namespace detail

template <class T> void sync(Serializer& s, T& v) {
    if constexpr (std::is_same_v<T, bool>) {
        uint8_t b = v ? 1 : 0;
        s.raw(&b, 1);
        if (s.loading()) {
            if (b > 1)
                s.fail("a flag that isn't 0 or 1");
            v = b == 1;
        }
    } else if constexpr (std::is_arithmetic_v<T>) {
        s.raw(&v, sizeof v);
    } else if constexpr (std::is_enum_v<T>) {
        // Not checked: an enum read from a save may hold any value of its
        // type; use syncEnum() where one out of range would do harm.
        auto u = static_cast<std::underlying_type_t<T>>(v);
        s.raw(&u, sizeof u);
        v = static_cast<T>(u);
    } else if constexpr (std::is_same_v<T, std::string>) {
        const uint32_t n = s.count(v.size());
        if (s.loading())
            v.assign(n, '\0');
        if (n)
            s.blob(v.data(), n);
    } else if constexpr (std::is_array_v<T>) {
        for (auto& e : v)
            sync(s, e);
    } else if constexpr (detail::IsArray<T>::value) {
        for (auto& e : v)
            sync(s, e);
    } else if constexpr (detail::IsVector<T>::value || detail::IsDeque<T>::value) {
        using E = typename T::value_type;
        const uint32_t n = s.count(v.size(), detail::minBytes<E>());
        if constexpr (detail::IsVector<T>::value && detail::kIsByte<E>) {
            // Bytes: the count was checked against what's left.
            if (s.loading())
                v.assign(n, E{});
            if (n)
                s.blob(v.data(), n);
        } else if (s.saving()) {
            if constexpr (std::is_same_v<T, std::vector<bool>>) {
                for (bool e : v)
                    sync(s, e);
            } else {
                for (auto& e : v)
                    sync(s, e);
            }
        } else {
            // Grown as the elements are read, so a count that the bytes
            // can't back fails before it costs memory.
            v.clear();
            if constexpr (detail::IsVector<T>::value) // (never more than the bytes left)
                v.reserve(std::min<size_t>(n, s.remaining() / std::max(sizeof(E), detail::minBytes<E>())));
            for (uint32_t i = 0; i < n && s.ok(); ++i) {
                if constexpr (std::is_same_v<T, std::vector<bool>>) {
                    bool b = false;
                    sync(s, b);
                    v.push_back(b);
                } else {
                    v.emplace_back();
                    sync(s, v.back());
                }
            }
        }
    } else if constexpr (detail::IsOptional<T>::value) {
        bool has = v.has_value();
        sync(s, has);
        if (s.loading())
            v = has ? std::make_optional<typename T::value_type>() : std::nullopt;
        if (has)
            sync(s, *v);
    } else if constexpr (detail::IsUnique<T>::value) {
        bool has = v != nullptr;
        sync(s, has);
        if (s.loading())
            v = has ? std::make_unique<typename T::element_type>() : nullptr;
        if (has)
            sync(s, *v);
    } else if constexpr (detail::IsPair<T>::value) {
        sync(s, v.first);
        sync(s, v.second);
    } else if constexpr (detail::IsMap<T>::value) {
        const uint32_t n =
            s.count(v.size(), detail::minBytes<typename T::key_type>() + detail::minBytes<typename T::mapped_type>());
        if (s.saving()) {
            for (auto& [k, val] : v) {
                auto key = k;
                sync(s, key);
                sync(s, val);
            }
        } else {
            v.clear();
            for (uint32_t i = 0; i < n && s.ok(); ++i) {
                typename T::key_type key{};
                sync(s, key);
                sync(s, v[key]);
            }
        }
    } else if constexpr (detail::IsSet<T>::value) {
        const uint32_t n = s.count(v.size(), detail::minBytes<typename T::key_type>());
        if (s.saving()) {
            for (auto k : v)
                sync(s, k);
        } else {
            v.clear();
            for (uint32_t i = 0; i < n && s.ok(); ++i) {
                typename T::key_type key{};
                sync(s, key);
                v.insert(key);
            }
        }
    } else {
        syncState(s, v); // the type's own, found by argument lookup
    }
}

// A pointer into a table the state is rebuilt from (an entry of the game's
// data, a list made on starting): saved as its place in `table` (a vector
// of the entries or of pointers to them), -1 for none.
template <class T, class Table> void syncIndex(Serializer& s, const T*& p, const Table& table) {
    int32_t index = -1;
    if (s.saving() && p)
        for (size_t i = 0; i < table.size(); ++i) {
            const T* entry;
            if constexpr (std::is_pointer_v<typename Table::value_type>)
                entry = table[i];
            else
                entry = &table[i];
            if (entry == p) {
                index = int32_t(i);
                break;
            }
        }
    s(index);
    if (s.loading()) {
        p = nullptr;
        if (index >= int32_t(table.size()))
            s.fail("an entry the game's data doesn't have");
        else if (index >= 0) {
            if constexpr (std::is_pointer_v<typename Table::value_type>)
                p = table[size_t(index)];
            else
                p = &table[size_t(index)];
        }
    }
}

// An enum that must be one of its first values, 0 .. `last` (a state
// switched on, an index into a table): a loader fails on any other.
template <class E> void syncEnum(Serializer& s, E& v, E last) {
    static_assert(std::is_enum_v<E>);
    using U = std::underlying_type_t<E>;
    U u = static_cast<U>(v);
    s(u);
    if (s.loading()) {
        if (std::cmp_less(u, 0) || u > static_cast<U>(last)) {
            s.fail("a value out of range");
            u = U(0);
        }
        v = static_cast<E>(u);
    }
}

// The file around an engine's state.
constexpr uint32_t kFormatVersion = 1;
constexpr int kThumbWidth = 160, kThumbHeight = 120;

struct Header {
    std::string engineId, gameId, target;
    uint32_t stateVersion = 0; // the engine's: a mismatch isn't loaded
    std::string description;
    int64_t created = 0; // seconds since 1970 (UTC)
    std::string reluvaVersion;
    int thumbWidth = 0, thumbHeight = 0;
    std::vector<uint8_t> thumbnail; // RGBA, thumbWidth * thumbHeight * 4
};

// A thumbnail of a screen (RGBA, `pitch` bytes a row), scaled to fit
// kThumbWidth x kThumbHeight.
void makeThumbnail(const uint8_t* rgba, int w, int h, int pitch, Header& out);

Error writeFile(const std::filesystem::path& file, const Header& header, const std::vector<uint8_t>& state);
// The header alone (for listing), or with the state.
Error readHeader(const std::filesystem::path& file, Header& header);
Error readFile(const std::filesystem::path& file, Header& header, std::vector<uint8_t>& state);

// Slots: numbered files in a game's save folder.
constexpr int kMaxSlot = 99;
std::filesystem::path slotPath(const std::filesystem::path& dir, int slot);
struct Slot {
    int number = 0;
    Header header; // read without the state
    std::string error; // why its header couldn't be read (empty if it could)
};
std::vector<Slot> listSlots(const std::filesystem::path& dir);
// The lowest slot without a file (0 if all are used).
int freeSlot(const std::filesystem::path& dir);

} // namespace reluva::save
