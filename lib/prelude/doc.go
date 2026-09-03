// Package prelude contains language builtin core libraries implemented.
//
// These modules form the language "prelude" and are loaded before user code.
// They provide:
//   - core types (array, map, set, channel, option, error)
//   - basic utilities (string ops, math primitives)
//   - language-required helpers
//
// Prelude code is compiled as if it were normal user modules,
// and made automatically available to all programs.
package prelude
