//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2022 - 2023 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_IDE_IDEBRIDGING
#define SWIFT_IDE_IDEBRIDGING

#include "swift/Basic/BasicBridging.h"
#include "swift/Basic/SourceLoc.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CAS/CASReference.h"
#include <vector>

enum class LabelRangeType {
  None,

  /// `foo([a: ]2) or .foo([a: ]String)`
  CallArg,

  /// `func([a b]: Int)`
  Param,

  /// `subscript([a a]: Int)`
  NoncollapsibleParam,

  /// `#selector(foo.func([a]:))`
  Selector,
};

enum class ResolvedLocContext { Default, Selector, Comment, StringLiteral };

struct ResolvedLoc {
  /// The range of the call's base name.
  swift::CharSourceRange range;

  // FIXME: (NameMatcher) We should agree on whether `labelRanges` contains the
  // colon or not
  /// The range of the labels.
  ///
  /// What the label range contains depends on the `labelRangeType`:
  /// - Labels of calls span from the label name (excluding trivia) to the end
  ///   of the colon's trivia.
  /// - Declaration labels contain the first name and the second name, excluding
  ///   the trivia on their sides
  /// - For function arguments that don't have a label, this is an empty range
  ///   that points to the start of the argument (exculding trivia).
  std::vector<swift::CharSourceRange> labelRanges;

  /// The in index in `labelRanges` that belongs to the first trailing closure
  /// or `llvm::None` if there is no trailing closure.
  llvm::Optional<unsigned> firstTrailingLabel;

  LabelRangeType labelType;

  /// Whether the location is in an active `#if` region or not.
  bool isActive;

  ResolvedLocContext context;

  SWIFT_NAME(
      "init(range:labelRanges:firstTrailingLabel:labelType:isActive:context:)")
  ResolvedLoc(BridgedCharSourceRange range, CharSourceRangeVector labelRanges,
              unsigned firstTrailingLabel, LabelRangeType labelType,
              bool isActive, ResolvedLocContext context);

  ResolvedLoc(swift::CharSourceRange range,
              std::vector<swift::CharSourceRange> labelRanges,
              llvm::Optional<unsigned> firstTrailingLabel,
              LabelRangeType labelType, bool isActive,
              ResolvedLocContext context);

  ResolvedLoc();
};

/// A heap-allocated `std::vector<ResoledLoc>` that can be represented by an
/// opaque pointer value.
///
/// This allows us to perform all the memory management for the heap-allocated
/// vector in C++. This makes it easier to manage because creating and
/// destorying an object in C++ is consistent with whether elements within the
/// vector are destroyed as well.
///
/// - Note: This should no longer be necessary when we use C++ to Swift interop.
///   In that case `swift_SwiftIDEUtilsBridging_runNameMatcher` can return a
///   `ResolvedLocVector`.
class BridgedResolvedLocVector {
  friend void *BridgedResolvedLocVector_getOpaqueValue(const BridgedResolvedLocVector &vector);

  std::vector<ResolvedLoc> *vector;

public:
  /// Create heap-allocaed vector with the same elements as `vector`.
  BridgedResolvedLocVector(const std::vector<ResolvedLoc> &vector);

  /// Create a `BridgedResolvedLocVector` from an opaque value obtained from
  /// `getOpaqueValue`.
  BridgedResolvedLocVector(void *opaqueValue);

  void push_back(const ResolvedLoc &Loc);

  /// Get the underlying vector.
  const std::vector<ResolvedLoc> &unbridged();

  /// Delete the heap-allocated memory owned by this object. Accessing
  /// `unbridged` is illegal after calling `destroy`.
  void destroy();

  SWIFT_IMPORT_UNSAFE
  void *getOpaqueValue() const;
};


SWIFT_NAME("BridgedResolvedLocVector.empty()")
BridgedResolvedLocVector BridgedResolvedLocVector_createEmpty();

typedef std::vector<BridgedSourceLoc> SourceLocVector;

/// Needed so that we can manually conform `SourceLocVectorIterator` to
/// `UnsafeCxxInputIterator` on the Swift side because the conformance is not
/// automatically generated by C++ interop by a Swift 5.8 compiler.
typedef std::vector<BridgedSourceLoc>::const_iterator SourceLocVectorIterator;

SWIFT_NAME("SourceLocVectorIterator.equals(_:_:)")
inline bool SourceLocVectorIterator_equal(const SourceLocVectorIterator &lhs,
                                          const SourceLocVectorIterator &rhs) {
  return lhs == rhs;
}

#ifdef __cplusplus
extern "C" {
#endif

/// Entry point to run the NameMatcher written in swift-syntax.
/// 
/// - Parameters:
///   - sourceFilePtr: A pointer to an `ExportedSourceFile`, used to access the
///     syntax tree
///   - locations: Pointer to a buffer of `BridgedSourceLoc` that should be
///     resolved by the name matcher.
///   - locationsCount: Number of elements in `locations`.
/// - Returns: The opaque value of a `BridgedResolvedLocVector`.
void *swift_SwiftIDEUtilsBridging_runNameMatcher(const void *sourceFilePtr,
                                                 BridgedSourceLoc *locations,
                                                 size_t locationsCount);
#ifdef __cplusplus
}
#endif

#endif
