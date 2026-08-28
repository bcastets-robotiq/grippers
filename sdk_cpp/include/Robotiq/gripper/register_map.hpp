// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

#pragma once

#include <cstddef>
#include <cstdint>

//! \ingroup register_map
//! \brief Byte and bit layout of the Robotiq 2F adaptive grippers'
//! (2F-85 / 2F-140 / Hand-E class) command and status blocks, as
//! published in the gripper's instruction manual.
//!
//! The blocks are byte-addressed:
//! \code
//!   Byte     Robot output (command)     Robot input (status)
//!   0        ACTION REQUEST             GRIPPER STATUS
//!   1        RESERVED                   RESERVED
//!   2        RESERVED                   FAULT STATUS
//!   3        POSITION REQUEST           POS REQUEST ECHO
//!   4        SPEED                      POSITION
//!   5        FORCE                      CURRENT
//!   6-15     RESERVED                   RESERVED
//! \endcode
//!
//! Two of those bytes are themselves packed with several fields — the
//! GRIPPER STATUS byte and the FAULT STATUS byte — so this namespace
//! also carries the bit masks and shifts needed to pull a field's raw
//! value out of its packed byte: `(byte & xMask) >> xShift`.
//!
//! Most consumers never need any of this directly: GripperCommand and
//! GripperStatus already expose the byte-level fields as named members,
//! and GripperStatusFlags / FaultStatus already expose the packed
//! bytes' sub-fields as accessors — activationState(), objectDetection(),
//! gripperFault(), controllerFault() — pre-masked and pre-shifted. Reach
//! for these constants only below that layer, e.g. decoding a raw byte
//! in a no-thread integration.
namespace Robotiq::register_map {

//! \name Block layout
//! How wide each block is, and how many of its leading bytes the manual
//! tables — the rest is reserved (see the byte table above).
//! \{
inline constexpr std::size_t kCommandBlockBytes = 16; //!< total width of the command block, in bytes
inline constexpr std::size_t kStatusBlockBytes = 16; //!< total width of the status block, in bytes
inline constexpr std::size_t kCommandDocumentedBytes = 6; //!< leading command-block bytes with a tabled field
inline constexpr std::size_t kStatusDocumentedBytes = 6; //!< leading status-block bytes with a tabled field
//! \}

//! \name GRIPPER STATUS byte — masks and shifts
//! Extract a field from byte 0 of the status block as
//! `(byte & xMask) >> xShift` (the single-bit flags gACT/gGTO have no
//! shift: the mask alone reads as 0 or 1).
//!
//! GripperStatusFlags already does this for you — reach for these
//! constants directly only when you have a raw status byte and no
//! GripperStatusFlags wrapping it, e.g. a no-thread integration decoding
//! bytes straight off the wire:
//! \code{.cpp}
//! uint8_t rawStatusByte = /* read from the wire */;
//! uint8_t gSTA = (rawStatusByte & Robotiq::register_map::kActivationStateMask)
//!                >> Robotiq::register_map::kActivationStateShift;
//! bool activated = (rawStatusByte & Robotiq::register_map::kActivationStatusMask) != 0;
//! \endcode
//! \{
inline constexpr uint8_t kActivationStatusMask = 0x01; //!< gACT
inline constexpr uint8_t kGoToEchoMask = 0x08; //!< gGTO
inline constexpr uint8_t kActivationStateMask = 0x30; //!< gSTA (bits 4-5)
inline constexpr uint8_t kObjectDetectionMask = 0xC0; //!< gOBJ (bits 6-7)
inline constexpr int kActivationStateShift = 4; //!< shift applied to gSTA after masking
inline constexpr int kObjectDetectionShift = 6; //!< shift applied to gOBJ after masking
//! \}

//! \name gSTA field values
//! What a masked-and-shifted gSTA decodes to. These mirror ActivationState
//! — GripperStatusFlags::activationState() returns that enum, built from
//! these same values — and exist here only for code decoding a raw byte
//! directly.
//! \{
inline constexpr uint8_t kActivationStateReset = 0x00; //!< see ActivationState::Reset
inline constexpr uint8_t kActivationStateInProgress = 0x01; //!< see ActivationState::InProgress
inline constexpr uint8_t kActivationStateReserved = 0x02; //!< see ActivationState::Reserved
inline constexpr uint8_t kActivationStateComplete = 0x03; //!< see ActivationState::Complete
//! \}

//! \name gOBJ field values
//! What a masked-and-shifted gOBJ decodes to. These mirror ObjectDetection
//! — GripperStatusFlags::objectDetection() returns that enum, built from
//! these same values — and exist here only for code decoding a raw byte
//! directly.
//! \{
inline constexpr uint8_t kObjectMoving = 0x00; //!< see ObjectDetection::Moving
inline constexpr uint8_t kObjectDetectedOpening = 0x01; //!< see ObjectDetection::DetectedWhileOpening
inline constexpr uint8_t kObjectDetectedClosing = 0x02; //!< see ObjectDetection::DetectedWhileClosing
inline constexpr uint8_t kObjectAtRequestedPosition = 0x03; //!< see ObjectDetection::AtRequestedPosition
//! \}

//! \name FAULT STATUS byte — masks and shift
//! Extract a field from byte 2 of the status block as `(byte & xMask) >> xShift`
//! (same pattern as the GRIPPER STATUS byte masks above; FaultStatus does
//! this for you unless you're decoding a raw byte directly).
//! \{
inline constexpr uint8_t kGripperFaultMask = 0x0F; //!< gFLT (low nibble)
inline constexpr uint8_t kControllerFaultMask = 0xF0; //!< kFLT (high nibble)
inline constexpr int kControllerFaultShift = 4; //!< shift applied to kFLT after masking
//! \}

} // namespace Robotiq::register_map
