#include "block.hpp"

void Block::set_type(Type type) { type_ = type; }

Block::Type Block::type() const { return type_; }
