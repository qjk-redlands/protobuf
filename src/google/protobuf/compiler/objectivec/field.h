// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FIELD_H__

#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

class FieldGenerator {
 public:
  static FieldGenerator* Make(const FieldDescriptor* field);

  virtual ~FieldGenerator() = default;

  FieldGenerator(const FieldGenerator&) = delete;
  FieldGenerator& operator=(const FieldGenerator&) = delete;

  // Exposed for subclasses to fill in.
  virtual void GenerateFieldStorageDeclaration(io::Printer* printer) const = 0;
  virtual void GeneratePropertyDeclaration(io::Printer* printer) const = 0;
  virtual void GeneratePropertyImplementation(io::Printer* printer) const = 0;

  // Called by GenerateFieldDescription, exposed for classes that need custom
  // generation.

  // Exposed for subclasses to extend, base does nothing.
  virtual void GenerateCFunctionDeclarations(io::Printer* printer) const;
  virtual void GenerateCFunctionImplementations(io::Printer* printer) const;

  // Exposed for subclasses, should always call it on the parent class also.
  virtual void DetermineForwardDeclarations(
      absl::btree_set<std::string>* fwd_decls,
      bool include_external_types) const;
  virtual void DetermineObjectiveCClassDefinitions(
      absl::btree_set<std::string>* fwd_decls) const;

  // Used during generation, not intended to be extended by subclasses.
  void GenerateFieldDescription(io::Printer* printer,
                                bool include_default) const;
  void GenerateFieldNumberConstant(io::Printer* printer) const;

  // Exposed to get and set the has bits information.
  virtual bool RuntimeUsesHasBit() const = 0;
  void SetRuntimeHasBit(int has_index);
  void SetNoHasBit();
  virtual int ExtraRuntimeHasBitsNeeded() const;
  virtual void SetExtraRuntimeHasBitsBase(int index_base);
  void SetOneofIndexBase(int index_base);

  std::string variable(const char* key) const {
    return variables_.find(key)->second;
  }

  bool needs_textformat_name_support() const {
    const std::string& field_flags = variable("fieldflags");
    return absl::StrContains(field_flags, "GPBFieldTextFormatNameCustom");
  }
  std::string generated_objc_name() const { return variable("name"); }
  std::string raw_field_name() const { return variable("raw_field_name"); }

 protected:
  explicit FieldGenerator(const FieldDescriptor* descriptor);

  virtual void FinishInitialization();
  bool WantsHasProperty() const;

  const FieldDescriptor* descriptor_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
};

class SingleFieldGenerator : public FieldGenerator {
 public:
  ~SingleFieldGenerator() override = default;

  SingleFieldGenerator(const SingleFieldGenerator&) = delete;
  SingleFieldGenerator& operator=(const SingleFieldGenerator&) = delete;

  void GenerateFieldStorageDeclaration(io::Printer* printer) const override;
  void GeneratePropertyDeclaration(io::Printer* printer) const override;

  void GeneratePropertyImplementation(io::Printer* printer) const override;

  bool RuntimeUsesHasBit() const override;

 protected:
  explicit SingleFieldGenerator(const FieldDescriptor* descriptor);
};

// Subclass with common support for when the field ends up as an ObjC Object.
class ObjCObjFieldGenerator : public SingleFieldGenerator {
 public:
  ~ObjCObjFieldGenerator() override = default;

  ObjCObjFieldGenerator(const ObjCObjFieldGenerator&) = delete;
  ObjCObjFieldGenerator& operator=(const ObjCObjFieldGenerator&) = delete;

  void GenerateFieldStorageDeclaration(io::Printer* printer) const override;
  void GeneratePropertyDeclaration(io::Printer* printer) const override;

 protected:
  explicit ObjCObjFieldGenerator(const FieldDescriptor* descriptor);
};

class RepeatedFieldGenerator : public ObjCObjFieldGenerator {
 public:
  ~RepeatedFieldGenerator() override = default;

  RepeatedFieldGenerator(const RepeatedFieldGenerator&) = delete;
  RepeatedFieldGenerator& operator=(const RepeatedFieldGenerator&) = delete;

  void GenerateFieldStorageDeclaration(io::Printer* printer) const override;
  void GeneratePropertyDeclaration(io::Printer* printer) const override;

  void GeneratePropertyImplementation(io::Printer* printer) const override;

  bool RuntimeUsesHasBit() const override;

  virtual void EmitArrayComment(io::Printer* printer) const;

 protected:
  explicit RepeatedFieldGenerator(const FieldDescriptor* descriptor);
  void FinishInitialization() override;
};

// Convenience class which constructs FieldGenerators for a Descriptor.
class FieldGeneratorMap {
 public:
  explicit FieldGeneratorMap(const Descriptor* descriptor);
  ~FieldGeneratorMap() = default;

  FieldGeneratorMap(const FieldGeneratorMap&) = delete;
  FieldGeneratorMap& operator=(const FieldGeneratorMap&) = delete;

  const FieldGenerator& get(const FieldDescriptor* field) const;

  // Assigns the has bits and returns the number of bits needed.
  int CalculateHasBits();

  void SetOneofIndexBase(int index_base);

  // Check if any field of this message has a non zero default.
  bool DoesAnyFieldHaveNonZeroDefault() const;

 private:
  const Descriptor* descriptor_;
  std::vector<std::unique_ptr<FieldGenerator>> field_generators_;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FIELD_H__
