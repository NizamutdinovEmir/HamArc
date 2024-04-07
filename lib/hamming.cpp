#include "hamming.h"

void EncodeHamming(std::vector<bool>& code) {
  size_t n = code.size();
  for (size_t control_bit = 1; control_bit < n - 1; control_bit *= 2) {
    size_t i = control_bit - 1;
    bool control_sum = false;
    while (i < n) {
      for (size_t j = i; (j < control_bit + i) && (j < n - 1); j++) {
        if (j == control_bit - 1) {
          continue;
        }
        control_sum = (control_sum != code[j]);
      }
      i += control_bit * 2;
    }
    code[control_bit - 1] = control_sum;
  }

  bool xor_bit = false;
  for (size_t i = 0; i < n - 1; i++) {
    xor_bit = (xor_bit != code[i]);
  }
  code[n - 1] = xor_bit;
}

bool DecodeHamming(std::vector<bool>& code) {
  std::vector<bool> control_bits;
  for (size_t control_bit = 1; control_bit < code.size() - 1; control_bit *= 2) {
    size_t i = control_bit - 1;
    bool control_sum = false;
    while (i < code.size()) {
      for (size_t j = i; (j < control_bit + i) && (j < code.size() - 1); j++) {
        if (j == control_bit - 1) {
          continue;
        }
        control_sum = (control_sum != code[j]);
      }
      i += control_bit * 2;
    }
    control_bits.push_back(control_sum);
  }
  size_t j = 1;
  size_t error_bit = 0;
  for (size_t i = 0; i < control_bits.size(); i++) {
    if (control_bits[i] != code[j - 1]) {
      error_bit += j;
    }
    j *= 2;
  }

  if (error_bit == 0) {
    return true;
  }

  bool xor_all = false;
  for (size_t i = 0; i < code.size(); i++) {
    xor_all = (xor_all != code[i]);
  }
  if (xor_all) {
    code[error_bit - 1] = !code[error_bit - 1];
    return true;
  } else {
    return false;
  }
}