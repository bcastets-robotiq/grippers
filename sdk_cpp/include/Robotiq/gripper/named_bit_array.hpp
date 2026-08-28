// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace Robotiq {

//! \ingroup utilities
//! \brief Manipulate individual bits of a value through a named-bit enum
//! whose enumerators are bit indices.
//!
//! Standard-layout and trivially copyable, so it composes into
//! wire-mapped blocks (see GripperCommand::action).
//!
//! \tparam BitEnum An enum (class) type whose enumerators are bit
//!         indices (0 = least significant bit) into the underlying value.
//!
//! \par Example
//! \code{.cpp}
//! enum class Flag : uint8_t { A = 0, B = 1 };
//! Robotiq::NamedBitArray<Flag> bits;
//! bits.set(Flag::A);
//! bits.set(Flag::B, false);
//! bool a = bits.get(Flag::A);   // true
//! \endcode
template <class BitEnum>
class NamedBitArray
{
public:
   //! The integer type backing \p BitEnum, and this array's storage.
   using Underlying = std::underlying_type_t<BitEnum>;

   //! All bits clear.
   constexpr NamedBitArray() = default;

   //! \param value The raw bit pattern to start from.
   constexpr explicit NamedBitArray(Underlying value)
      : _value(value)
   {
   }

   //! \param bit Which bit to read.
   //! \return true if \p bit is set.
   //! \note [[nodiscard]]: a pure accessor with no side effects; calling it
   //!       only to discard the result is always a mistake.
   [[nodiscard]] constexpr bool get(BitEnum bit) const { return (_value & mask(bit)) != 0; }

   //! \param bit Which bit to set.
   constexpr void set(BitEnum bit) { _value = static_cast<Underlying>(_value | mask(bit)); }

   //! \param bit Which bit to clear.
   constexpr void unset(BitEnum bit) { _value = static_cast<Underlying>(_value & ~mask(bit)); }

   //! \param bit Which bit to change.
   //! \param on true to set \p bit, false to clear it.
   constexpr void set(BitEnum bit, bool on) { on ? set(bit) : unset(bit); }

   //! \return The raw bit pattern.
   //! \note [[nodiscard]]: a pure accessor with no side effects; calling it
   //!       only to discard the result is always a mistake.
   [[nodiscard]] constexpr Underlying value() const { return _value; }

   //! \return true if both arrays hold the same bit pattern.
   //! \note [[nodiscard]]: a pure comparison with no side effects; calling
   //!       it only to discard the result is always a mistake.
   [[nodiscard]] constexpr bool operator==(NamedBitArray other) const { return _value == other._value; }
   //! \return true if the bit patterns differ.
   //! \note [[nodiscard]]: a pure comparison with no side effects; calling
   //!       it only to discard the result is always a mistake.
   [[nodiscard]] constexpr bool operator!=(NamedBitArray other) const { return _value != other._value; }

private:
   static constexpr Underlying mask(BitEnum bit)
   {
      assert(static_cast<unsigned>(bit) < std::numeric_limits<Underlying>::digits);
      return static_cast<Underlying>(1U << static_cast<unsigned>(bit));
   }

   Underlying _value = 0;
};

namespace detail {
enum class SampleBit : uint8_t
{
   Zero = 0
};
static_assert(std::is_standard_layout_v<NamedBitArray<SampleBit>>
                 && std::is_trivially_copyable_v<NamedBitArray<SampleBit>> && sizeof(NamedBitArray<SampleBit>) == 1,
              "NamedBitArray must stay byte-sized and wire-composable");
} // namespace detail

} // namespace Robotiq
