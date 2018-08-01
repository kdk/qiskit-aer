/**
 * Copyright 2018, IBM.
 *
 * This source code is licensed under the Apache License, Version 2.0 found in
 * the LICENSE.txt file in the root directory of this source tree.
 */

/**
 * @file    operations.hpp
 * @brief   Simualator operations
 * @author  Christopher J. Wood <cjwood@us.ibm.com>
 */

#ifndef _aer_framework_operations_hpp_
#define _aer_framework_operations_hpp_

#include <algorithm>  // for std::copy
#include <stdexcept>
#include <string>
#include <vector>

#include "framework/types.hpp"
#include "framework/json.hpp"

namespace AER {
namespace Operations {

//------------------------------------------------------------------------------
// Op Class
//------------------------------------------------------------------------------

struct Op {
  std::string name;         // operation name
  bool conditional = false; // is gate conditional gate
  reg_t qubits;             // (opt) qubits operation acts on

  reg_t memory;             // (opt) register operation it acts on (measure)
  reg_t registers;          // (opt) register locations it acts on (measure, conditional)
  uint_t conditional_reg;   // (opt) the (single) register location to look up for conditional
  
  // Data structures for parameters
  std::vector<std::string> params_string;       // string params
  std::vector<double> params_double;       // double params
  std::vector<complex_t> params_complex;   // complex double params
  std::vector<cvector_t> params_cvector;   // complex vector params
  std::vector<cmatrix_t> params_cmatrix;   // complex matrix params
  std::vector<reg_t> params_reg;           // reg_t params

};

//------------------------------------------------------------------------------
// Error Checking
//------------------------------------------------------------------------------

inline void check_qubits(const reg_t &qubits) {
  // Check qubits isn't empty
  if (qubits.empty()) {
    throw std::invalid_argument("Invalid operation (\"qubits\" are empty)");
  }
  // Check qubits are unique
  auto cpy = qubits;
  std::unique(cpy.begin(), cpy.end());
  if (cpy != qubits) {
    throw std::invalid_argument("Invalid operation (\"qubits\" are not unique)");
  }
};

//------------------------------------------------------------------------------
// JSON conversion
//------------------------------------------------------------------------------

// Main JSON deserialization functions
Op json_to_op(const json_t &js); // Patial TODO
inline void from_json(const json_t &js, Op &op) {op = json_to_op(js);};

// Helper deserialization functions
Op json_to_op_gate(const json_t &js);
Op json_to_op_measure(const json_t &js);
Op json_to_op_reset(const json_t &js);

Op json_to_op_mat(const json_t &js);
Op json_to_op_dmat(const json_t &js);

// Observables
Op json_to_op_obs(const json_t &js);
Op json_to_op_obs_mat(const json_t &js);
Op json_to_op_obs_dmat(const json_t &js);
Op json_to_op_obs_vec(const json_t &js);
Op json_to_op_obs_pauli(const json_t &js); 
Op json_to_op_probs(const json_t &js);

// Special
Op json_to_op_snapshot(const json_t &js);
Op json_to_op_kraus(const json_t &js);
Op json_to_op_roerror(const json_t &js); // TODO
Op json_to_op_bfunc(const json_t &js); // TODO


//------------------------------------------------------------------------------
// Implementation: JSON deserialization
//------------------------------------------------------------------------------

// TODO: convert if-else to switch
Op json_to_op(const json_t &js) {
  // load operation identifier
  std::string name;
  JSON::get_value(name, "name", js);
  if (name.empty()) {
    throw std::invalid_argument("Invalid gate operation: \"name\" is empty.");
  }
  // Measure & Reset
  if (name == "measure")
    return json_to_op_measure(js);
  if (name == "reset")
    return json_to_op_reset(js);
  // Arbitrary matrix gates
  if (name == "mat")
    return json_to_op_mat(js);
  if (name == "dmat")
    return json_to_op_dmat(js);
  // Observables
  if (name == "probs")
    return json_to_op_probs(js);
  if (name == "obs_pauli")
    return json_to_op_obs_pauli(js);
  /* TODO
  if (name == "obs_mat")
    return json_to_op_obs_mat(js);
  if (name == "obs_dmat")
    return json_to_op_obs_dmat(js);
  if (name == "obs_vec")
    return json_to_op_obs_vec(js);
  */
  // Special
  if (name == "snapshot")
    return json_to_op_snapshot(js);
  if (name == "kraus")
    return json_to_op_kraus(js);
  /* TODO: the following aren't implemented yet!
  if (name == "bfunc")
    return json_to_op_bfunc(js);
  if (name == "roerror")
    return json_to_op_roerror(js);
  */
  // Gates
  return json_to_op_gate(js);
}


Op json_to_op_gate(const json_t &js) {
  Op op;
  // Load name identifier
  JSON::get_value(op.name, "name", js);
  if (op.name.empty()) {
    throw std::invalid_argument("Invalid gate operation: \"name\" are empty.");
  }
  // Load qubits
  JSON::get_value(op.qubits, "qubits", js);
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid gate operation: \"qubits\" are empty.");
  }
  // Load double params (if present)
  JSON::get_value(op.params_double, "params", js);
  return op;
}


Op json_to_op_measure(const json_t &js) {
  Op op;
  op.name = "measure";
  // Load qubits
  JSON::get_value(op.qubits, "qubits", js);
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid measure operation: \"qubits\" are empty.");
  }
  // Load memory (if present)
  JSON::get_value(op.memory, "memory", js);
  if (op.memory.empty() == false && op.memory.size() != op.qubits.size()) {
    throw std::invalid_argument("Invalid measure operation: \"memory\" and \"qubits\" are different lengths.");
  }
  // Load registers (if present)
  JSON::get_value(op.registers, "register", js);
  if (op.registers.empty() == false && op.registers.size() != op.qubits.size()) {
    throw std::invalid_argument("Invalid measure operation: \"register\" and \"qubits\" are different lengths.");
  }
  return op;
}


Op json_to_op_reset(const json_t &js) {
  Op op;
  op.name = "reset";
  // Load qubits
  JSON::get_value(op.qubits, "qubits", js);
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid reset operation: \"qubits\" are empty.");
  }
  // Load double params for reset state (if present)
  JSON::get_value(op.params_double, "params", js);
  if (op.params_double.empty()) {
    // If not present default reset to all zero state
    op.params_double = rvector_t(op.qubits.size(), 0.);
  }
  if (op.params_double.size() != op.qubits.size()) {
    throw std::invalid_argument("Invalid reset operation: \"params\" and \"qubits\" are different lengths.");
  }
  return op;
}


Op json_to_op_snapshot(const json_t &js) {
  Op op;
  op.name = "snapshot";
  // Load double params for reset state (if present)
  JSON::get_value(op.params_string, "params", js);
  if (op.params_string.size() == 1) {
    // add default snapshot type if not specified
    op.params_string.push_back("default");
  }
  return op;
}


Op json_to_op_mat(const json_t &js) {
  Op op;
  op.name = "mat";
  // load qubits
  JSON::get_value(op.qubits, "qubits", js);
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid obs_mat operation: \"qubits\" are empty.");
  }
  // load matrices
  cmatrix_t tmp;
  JSON::get_value(tmp, "params", js);
  op.params_cmatrix.emplace_back(std::move(tmp));
  return op;
}


Op json_to_op_dmat(const json_t &js) {
  Op op;
  op.name = "dmat";
  // load qubits
  JSON::get_value(op.qubits, "qubits", js);
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid obs_mat operation: \"qubits\" are empty.");
  }
  // load diagonal
  JSON::get_value(op.params_complex, "params", js);
  return op;
}





Op json_to_op_kraus(const json_t &js) {
  Op op;
  op.name = "kraus";
  // load qubits
  JSON::get_value(op.qubits, "qubits", js);
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid obs_mat operation: \"qubits\" are empty.");
  }
  // load matrices
  JSON::get_value(op.params_cmatrix, "params", js);
  return op;
}

//------------------------------------------------------------------------------
// Observables JSON deserialization
//------------------------------------------------------------------------------

// Measurement probabilities observables
Op json_to_op_probs(const json_t &js) {
  Op op;
  op.name = "probs";
  JSON::get_value(op.qubits, "qubits", js);

  // Error handling
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid obs_mat operation (\"qubits\" are empty).");
  }
  return op;
}


Op json_to_op_obs(const json_t &js) {
  std::string name;
  JSON::get_value(name, "name", js);
  if (name == "obs_pauli")
    return json_to_op_obs_pauli(js);
  if (name == "obs_mat")
    return json_to_op_obs_mat(js);
  if (name == "obs_dmat")
    return json_to_op_obs_dmat(js);
  if (name == "obs_vec")
    return json_to_op_obs_vec(js);
  throw std::invalid_argument("Invalid observable operation.");  
}


Op json_to_op_obs_pauli(const json_t &js) {
  Op op;
  op.name = "obs_pauli";
  JSON::get_value(op.qubits, "qubits", js);
  JSON::get_value(op.params_string, "params", js);
  JSON::get_value(op.params_complex, "coeffs", js);

  // Sort qubits and params (this could be improved)
  // It is currently needed for caching strings for
  // observables engine
  reg_t unsorted = op.qubits;
  std::sort(op.qubits.begin(), op.qubits.end());
  for (auto &s : op.params_string) {
    std::string srt;
    for (const auto q: op.qubits) {
      auto pos = std::distance(unsorted.begin(),
                               std::find(unsorted.begin(), unsorted.end(), q));
      srt.push_back(s[pos]);
    }
    s = srt;
  }
  // Error handling
  if (op.qubits.empty()) {
    throw std::invalid_argument("Invalid obs_pauli operation (\"qubits\" are empty).");
  }
  if (op.params_string.empty()) {
    throw std::invalid_argument("Invalid obs_pauli operation (\"params\" are empty).");
  }
  for (const auto &s: op.params_string) {
    if (s.size() != op.qubits.size())
      throw std::invalid_argument("Invalid obs_pauli operation (\"params\" string incorrect length for qubit number).");
  }
  if (op.params_complex.size() != op.params_string.size()) {
    throw std::invalid_argument("Invalid obs_pauli operation (length \"coeffs\" != length \"params\").");
  }
  return op;
}


Op json_to_op_obs_mat(const json_t &js) {
  Op op;
  op.name = "obs_mat";
  // load qubits
  JSON::get_value(op.qubits, "qubits", js); 

  // Sub qubits
  JSON::get_value(op.params_reg, "sub_qubits", js);
  JSON::get_value(op.params_cmatrix, "sub_params", js);

  // Validation and error handling
  check_qubits(op.qubits); // check qubits field
  
  // Check that sub qubits contains all qubits only once
  size_t num_sub = 0;
  std::set<uint_t> qset;
  for (const auto &reg : op.params_reg) {
    num_sub += reg.size();
    qset.insert(reg.begin(), reg.end());
  }
  if (qset.size() != op.qubits.size() || num_sub != op.qubits.size()) {
    throw std::invalid_argument("Invalid obs_mat operations (sub_qubits is not compatible with qubits specification)");
  }
  // Check correct number of matrices
  if (op.params_reg.size() != op.params_cmatrix.size()) {
    throw std::invalid_argument("Invalid obs_mat (sub_qubits do not match sub_params).");
  }
  // Should we check dimension of the matrices here?
  return op;
}


Op json_to_op_obs_dmat(const json_t &js) {
  Op op;
  op.name = "obs_dmat";
  // load qubits
  JSON::get_value(op.qubits, "qubits", js); 

  // Sub qubits
  JSON::get_value(op.params_reg, "sub_qubits", js);
  JSON::get_value(op.params_cvector, "sub_params", js);

  // Validation and error handling
  check_qubits(op.qubits); // check qubits field
  
  // Check that sub qubits contains all qubits only once
  size_t num_sub = 0;
  std::set<uint_t> qset;
  for (const auto &reg : op.params_reg) {
    num_sub += reg.size();
    qset.insert(reg.begin(), reg.end());
  }
  if (qset.size() != op.qubits.size() || num_sub != op.qubits.size()) {
    throw std::invalid_argument("Invalid obs_dmat operations (sub_qubits is not compatible with qubits specification)");
  }
  // Check correct number of matrices
  if (op.params_reg.size() != op.params_cvector.size()) {
    throw std::invalid_argument("Invalid obs_dmat (sub_qubits do not match sub_params).");
  }
  // Should we check dimension of the matrices here?
  return op;
}


Op json_to_op_obs_vec(const json_t &js) {
  Op op;
  op.name = "obs_vec";
  // load qubits
  JSON::get_value(op.qubits, "qubits", js); 

  // Sub qubits
  JSON::get_value(op.params_reg, "sub_qubits", js);
  JSON::get_value(op.params_cvector, "sub_params", js);

  // Validation and error handling
  check_qubits(op.qubits); // check qubits field
  
  // Check that sub qubits contains all qubits only once
  size_t num_sub = 0;
  std::set<uint_t> qset;
  for (const auto &reg : op.params_reg) {
    num_sub += reg.size();
    qset.insert(reg.begin(), reg.end());
  }
  if (qset.size() != op.qubits.size() || num_sub != op.qubits.size()) {
    throw std::invalid_argument("Invalid obs_vec operations (sub_qubits is not compatible with qubits specification)");
  }
  // Check correct number of matrices
  if (op.params_reg.size() != op.params_cvector.size()) {
    throw std::invalid_argument("Invalid obs_vec (sub_qubits do not match sub_params).");
  }
  // Should we check dimension of the matrices here?
  return op;
}

//------------------------------------------------------------------------------
} // end namespace Operations
//------------------------------------------------------------------------------
} // end namespace AER
//------------------------------------------------------------------------------
#endif