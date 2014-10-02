#include "CLSmith/Globals.h"
#include "CLSmith/StatementAtomicReduction.h"
#include "CLSmith/StatementBarrier.h"

#include "CVQualifiers.h"
#include "Expression.h"
#include "FactMgr.h"
#include "random.h"
#include "Type.h"
#include "VariableSelector.h"

namespace CLSmith {
  
namespace {
// Variable to store the value of a modified variable
Variable* hash_buffer = NULL;
}
  
StatementAtomicReduction* StatementAtomicReduction::make_random(CGContext &cg_context) {
  const Type* type = get_int_type();
  Expression* expr = Expression::make_random(cg_context, type);
  vector<const Variable*> invalid_vars;
  Variable* var = NULL;
  while (var == NULL) {
    var = VariableSelector::select(Effect::WRITE, cg_context, type, 
        new CVQualifiers(std::vector<bool>({false}), 
        std::vector<bool>({false})), invalid_vars, eDerefExact);
    invalid_vars.push_back(var);
  }
  const bool has_global = (const bool) rnd_upto(2);
  AtomicOp op = (AtomicOp) rnd_upto(kXor);
  return new StatementAtomicReduction(var, expr, has_global, 
                                      cg_context.get_current_block(), op);
}

Variable* StatementAtomicReduction::get_hash_buffer() {
  if (hash_buffer == NULL)
    hash_buffer = Variable::CreateVariable("v_collective", 
        &Type::get_simple_type(eUInt), Constant::make_int(0), 
        new CVQualifiers(std::vector<bool>({false}), 
        std::vector<bool>({false})));
  return hash_buffer;
}

void StatementAtomicReduction::RecordBuffer() {
  Variable* buf = get_hash_buffer();
  if (buf != NULL)
    VariableSelector::GetGlobalVariables()->push_back(buf);
}
  
void StatementAtomicReduction::Output(std::ostream& out, FactMgr* fm, int indent) const {
  output_tab(out, indent);
  out << "atomic_";
  switch (op_) {
    case (kAdd)  : out << "add" ; break;
    case (kSub)  : out << "sub" ; break;
    case (kXchg) : out << "xchg"; break;
    case (kMin)  : out << "min" ; break;
    case (kMax)  : out << "max" ; break;
    case (kAnd)  : out << "and" ; break;
    case (kOr)   : out << "or"  ; break;
    case (kXor)  : out << "xor" ; break;
  }
  out << "(" << var_->to_string() << ", ";
  expr_->Output(out);
  if (add_global_)
    out << " + linear_global_id()";
  out << ");" << std::endl;
  output_tab(out, indent);
  StatementBarrier::OutputBarrier(out); out << std::endl;
  output_tab(out, indent);
  out << "if (linear_global_id() == 0)" << std::endl;
  output_tab(out, indent + 1);
  out << get_hash_buffer()->to_string() <<  "+=" << var_->to_string() << ";" << std::endl;
  output_tab(out, indent);
  StatementBarrier::OutputBarrier(out); out << std::endl;
}

}