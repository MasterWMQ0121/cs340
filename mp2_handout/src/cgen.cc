/*********************************************************************
 Intermediate code generator for COOL: SKELETON

 Read the comments carefully and add code to build an LLVM program
*********************************************************************/

#define EXTERN
#include "cgen.h"
#include <fstream>
#include <sstream>
#include <string>

extern int cgen_debug, curr_lineno;

/*********************************************************************
 For convenience, a large number of symbols are predefined here.
 These symbols include the primitive type and method names, as well
 as fixed names used by the runtime system. Feel free to add your
 own definitions as you see fit.
*********************************************************************/
EXTERN Symbol
    // required classes
    Object,
    IO, String, Int, Bool, Main,

    // class methods
    cool_abort, type_name, cool_copy, out_string, out_int, in_string, in_int,
    length, concat, substr,

    // class members
    val,

    // special symbols
    No_class,  // symbol that can't be the name of any user-defined class
    No_type,   // If e : No_type, then no code is generated for e.
    SELF_TYPE, // Special code is generated for new SELF_TYPE.
    self,      // self generates code differently than other references

    // extras
    arg, arg2, newobj, Mainmain, prim_string, prim_int, prim_bool;

// Initializing the predefined symbols.
static void initialize_constants(void) {
  Object = idtable.add_string("Object");
  IO = idtable.add_string("IO");
  String = idtable.add_string("String");
  Int = idtable.add_string("Int");
  Bool = idtable.add_string("Bool");
  Main = idtable.add_string("Main");

  cool_abort = idtable.add_string("abort");
  type_name = idtable.add_string("type_name");
  cool_copy = idtable.add_string("copy");
  out_string = idtable.add_string("out_string");
  out_int = idtable.add_string("out_int");
  in_string = idtable.add_string("in_string");
  in_int = idtable.add_string("in_int");
  length = idtable.add_string("length");
  concat = idtable.add_string("concat");
  substr = idtable.add_string("substr");

  val = idtable.add_string("val");

  No_class = idtable.add_string("_no_class");
  No_type = idtable.add_string("_no_type");
  SELF_TYPE = idtable.add_string("SELF_TYPE");
  self = idtable.add_string("self");

  arg = idtable.add_string("arg");
  arg2 = idtable.add_string("arg2");
  newobj = idtable.add_string("_newobj");
  Mainmain = idtable.add_string("main");
  prim_string = idtable.add_string("sbyte*");
  prim_int = idtable.add_string("int");
  prim_bool = idtable.add_string("bool");
}

/*********************************************************************

  CgenClassTable methods

*********************************************************************/

// CgenClassTable constructor orchestrates all code generation
CgenClassTable::CgenClassTable(Classes classes, std::ostream &s)
    : nds(0), current_tag(0) {
  if (cgen_debug)
    std::cerr << "Building CgenClassTable" << std::endl;
  ct_stream = &s;
  // Make sure we have a scope, both for classes and for constants
  enterscope();

  // Create an inheritance tree with one CgenNode per class.
  install_basic_classes();
  install_classes(classes);
  build_inheritance_tree();

  // First pass
  setup();

  // Second pass
  code_module();
  // Done with code generation: exit scopes
  exitscope();
}

// Creates AST nodes for the basic classes and installs them in the class list
void CgenClassTable::install_basic_classes() {
  // The tree package uses these globals to annotate the classes built below.
  curr_lineno = 0;
  Symbol filename = stringtable.add_string("<basic class>");

  //
  // A few special class names are installed in the lookup table but not
  // the class list. Thus, these classes exist, but are not part of the
  // inheritance hierarchy.

  // No_class serves as the parent of Object and the other special classes.
  Class_ noclasscls = class_(No_class, No_class, nil_Features(), filename);
  install_special_class(new CgenNode(noclasscls, CgenNode::Basic, this));
  delete noclasscls;

#ifdef MP3
  // SELF_TYPE is the self class; it cannot be redefined or inherited.
  Class_ selftypecls = class_(SELF_TYPE, No_class, nil_Features(), filename);
  install_special_class(new CgenNode(selftypecls, CgenNode::Basic, this));
  delete selftypecls;
  //
  // Primitive types masquerading as classes. This is done so we can
  // get the necessary Symbols for the innards of String, Int, and Bool
  //
  Class_ primstringcls =
      class_(prim_string, No_class, nil_Features(), filename);
  install_special_class(new CgenNode(primstringcls, CgenNode::Basic, this));
  delete primstringcls;
#endif
  Class_ primintcls = class_(prim_int, No_class, nil_Features(), filename);
  install_special_class(new CgenNode(primintcls, CgenNode::Basic, this));
  delete primintcls;
  Class_ primboolcls = class_(prim_bool, No_class, nil_Features(), filename);
  install_special_class(new CgenNode(primboolcls, CgenNode::Basic, this));
  delete primboolcls;
  //
  // The Object class has no parent class. Its methods are
  //    cool_abort() : Object    aborts the program
  //    type_name() : Str        returns a string representation of class name
  //    copy() : SELF_TYPE       returns a copy of the object
  //
  // There is no need for method bodies in the basic classes---these
  // are already built in to the runtime system.
  //
  Class_ objcls = class_(
      Object, No_class,
      append_Features(
          append_Features(single_Features(method(cool_abort, nil_Formals(),
                                                 Object, no_expr())),
                          single_Features(method(type_name, nil_Formals(),
                                                 String, no_expr()))),
          single_Features(
              method(cool_copy, nil_Formals(), SELF_TYPE, no_expr()))),
      filename);
  install_class(new CgenNode(objcls, CgenNode::Basic, this));
  delete objcls;

  //
  // The Int class has no methods and only a single attribute, the
  // "val" for the integer.
  //
  Class_ intcls = class_(
      Int, Object, single_Features(attr(val, prim_int, no_expr())), filename);
  install_class(new CgenNode(intcls, CgenNode::Basic, this));
  delete intcls;

  //
  // Bool also has only the "val" slot.
  //
  Class_ boolcls = class_(
      Bool, Object, single_Features(attr(val, prim_bool, no_expr())), filename);
  install_class(new CgenNode(boolcls, CgenNode::Basic, this));
  delete boolcls;

#ifdef MP3
  //
  // The class String has a number of slots and operations:
  //       val                                  the string itself
  //       length() : Int                       length of the string
  //       concat(arg: Str) : Str               string concatenation
  //       substr(arg: Int, arg2: Int): Str     substring
  //
  Class_ stringcls =
      class_(String, Object,
             append_Features(
                 append_Features(
                     append_Features(
                         single_Features(attr(val, prim_string, no_expr())),
                         single_Features(
                             method(length, nil_Formals(), Int, no_expr()))),
                     single_Features(method(concat,
                                            single_Formals(formal(arg, String)),
                                            String, no_expr()))),
                 single_Features(
                     method(substr,
                            append_Formals(single_Formals(formal(arg, Int)),
                                           single_Formals(formal(arg2, Int))),
                            String, no_expr()))),
             filename);
  install_class(new CgenNode(stringcls, CgenNode::Basic, this));
  delete stringcls;
#endif

#ifdef MP3
  //
  // The IO class inherits from Object. Its methods are
  //        out_string(Str) : SELF_TYPE          writes a string to the output
  //        out_int(Int) : SELF_TYPE               "    an int    "  "     "
  //        in_string() : Str                    reads a string from the input
  //        in_int() : Int                         "   an int     "  "     "
  //
  Class_ iocls = class_(
      IO, Object,
      append_Features(
          append_Features(
              append_Features(
                  single_Features(method(out_string,
                                         single_Formals(formal(arg, String)),
                                         SELF_TYPE, no_expr())),
                  single_Features(method(out_int,
                                         single_Formals(formal(arg, Int)),
                                         SELF_TYPE, no_expr()))),
              single_Features(
                  method(in_string, nil_Formals(), String, no_expr()))),
          single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
      filename);
  install_class(new CgenNode(iocls, CgenNode::Basic, this));
  delete iocls;
#endif
}

// install_classes enters a list of classes in the symbol table.
void CgenClassTable::install_classes(Classes cs) {
  for (auto cls : cs) {
    install_class(new CgenNode(cls, CgenNode::NotBasic, this));
  }
}

// Add this CgenNode to the class list and the lookup table
void CgenClassTable::install_class(CgenNode *nd) {
  Symbol name = nd->get_name();
  if (!this->find(name)) {
    // The class name is legal, so add it to the list of classes
    // and the symbol table.
    nds.push_back(nd);
    this->insert(name, nd);
  }
}

// Add this CgenNode to the special class list and the lookup table
void CgenClassTable::install_special_class(CgenNode *nd) {
  Symbol name = nd->get_name();
  if (!this->find(name)) {
    // The class name is legal, so add it to the list of special classes
    // and the symbol table.
    special_nds.push_back(nd);
    this->insert(name, nd);
  }
}

// CgenClassTable::build_inheritance_tree
void CgenClassTable::build_inheritance_tree() {
  for (auto node : nds)
    set_relations(node);
}

// CgenClassTable::set_relations
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table. Parent and child pointers are added as appropriate.
//
void CgenClassTable::set_relations(CgenNode *nd) {
  Symbol parent = nd->get_parent();
  auto parent_node = this->find(parent);
  if (!parent_node) {
    throw std::runtime_error("Class " + nd->get_name()->get_string() +
                             " inherits from an undefined class " +
                             parent->get_string());
  }
  nd->set_parent(parent_node);
}

// Sets up declarations for extra functions needed for code generation
// You should not need to modify this code for MP2
void CgenClassTable::setup_external_functions() {
  ValuePrinter vp(*ct_stream);
  // setup function: external int strcmp(sbyte*, sbyte*)
  op_type i32_type(INT32), i8ptr_type(INT8_PTR), vararg_type(VAR_ARG);
  std::vector<op_type> strcmp_args;
  strcmp_args.push_back(i8ptr_type);
  strcmp_args.push_back(i8ptr_type);
  vp.declare(*ct_stream, i32_type, "strcmp", strcmp_args);

  // setup function: external int printf(sbyte*, ...)
  std::vector<op_type> printf_args;
  printf_args.push_back(i8ptr_type);
  printf_args.push_back(vararg_type);
  vp.declare(*ct_stream, i32_type, "printf", printf_args);

  // setup function: external void abort(void)
  op_type void_type(VOID);
  std::vector<op_type> abort_args;
  vp.declare(*ct_stream, void_type, "abort", abort_args);

  // setup function: external i8* malloc(i32)
  std::vector<op_type> malloc_args;
  malloc_args.push_back(i32_type);
  vp.declare(*ct_stream, i8ptr_type, "malloc", malloc_args);

#ifdef MP3
  // TODO: add code here
#endif
}

void CgenClassTable::setup_classes(CgenNode *c, int depth) {
  c->setup(current_tag++, depth);
  for (auto child : c->get_children()) {
    setup_classes(child, depth + 1);
  }
  c->set_max_child(current_tag - 1);
}

// The code generation first pass. Define these two functions to traverse
// the tree and setup each CgenNode
void CgenClassTable::setup() {
  setup_external_functions();
  setup_classes(root(), 0);
}

// The code generation second pass. Add code here to traverse the tree and
// emit code for each CgenNode
void CgenClassTable::code_module() {
  code_constants();

#ifndef MP3
  // This must be after code_constants() since that emits constants
  // needed by the code() method for expressions
  CgenNode *mainNode = getMainmain(root());
  mainNode->codeGenMainmain();
#endif
  code_main();

#ifdef MP3
  code_classes(root());
#endif
}

#ifdef MP3
void CgenClassTable::code_classes(CgenNode *c) {
  // TODO: add code here
}
#endif

// Create global definitions for constant Cool objects
void CgenClassTable::code_constants() {
#ifdef MP3
  // TODO: add code here
#endif
}

// Create LLVM entry point. This function will initiate our Cool program
// by generating the code to execute (new Main).main()
//
void CgenClassTable::code_main(){
// TODO: add code here
  ValuePrinter vp(*ct_stream);
  op_arr_type i8_arr_type(INT8, 25);

  const_value content(i8_arr_type, "Main.main() returned %d\n", true);

  vp.init_constant(*ct_stream, ".str", content);

// Define a function main that has no parameters and returns an i32
  vp.define(*ct_stream, op_type(INT32), "main", {});

// Define an entry basic block
  vp.begin_block("entry");

// Call Main_main(). This returns int for phase 1, Object for phase 2
  operand Main_main = vp.call({}, op_type(INT32), "Main_main", true, {});  

#ifdef MP3
// MP3
#else
// MP2
// Get the address of the string "Main_main() returned %d\n" using
// getelementptr
  operand ptr = vp.getelementptr(i8_arr_type, global_value(i8_arr_type.get_ptr_type(), ".str"), int_value(0), int_value(0), op_type(INT8_PTR));

// Call printf with the string address of "Main_main() returned %d\n"
// and the return value of Main_main() as its arguments
  std::vector<op_type> types = {op_type(INT8_PTR), op_type(VAR_ARG)};
  std::vector<operand> args = {ptr, Main_main};
  vp.call(types, op_type(INT32), "printf", true, args);

// Insert return 0
  vp.ret(*ct_stream, int_value(0));

// end define
  vp.end_define();

#endif
}

// Get the root of the class tree.
CgenNode *CgenClassTable::root() {
  auto root = this->find(Object);
  if (!root) {
    throw std::runtime_error("Class Object is not defined.");
  }
  return root;
}

#ifndef MP3
// Special-case functions used for the method Int Main::main() for
// MP2 only.
CgenNode *CgenClassTable::getMainmain(CgenNode *c) {
  if (c && !c->basic())
    return c; // Found it!
  for (auto child : c->get_children()) {
    if (CgenNode *foundMain = this->getMainmain(child))
      return foundMain; // Propagate it up the recursive calls
  }
  return 0; // Make the recursion continue
}
#endif

/*********************************************************************

  StrTable / IntTable methods

 Coding string, int, and boolean constants

 Cool has three kinds of constants: strings, ints, and booleans.
 This section defines code generation for each type.

 All string constants are listed in the global "stringtable" and have
 type stringEntry. stringEntry methods are defined both for string
 constant definitions and references.

 All integer constants are listed in the global "inttable" and have
 type IntEntry. IntEntry methods are defined for Int constant references only.

 Since there are only two Bool values, there is no need for a table.
 The two booleans are represented by instances of the class BoolConst,
 which defines the definition and reference methods for Bools.

*********************************************************************/

// Create definitions for all String constants
void StrTable::code_string_table(std::ostream &s, CgenClassTable *ct) {
  for (auto &[_, entry] : this->_table) {
    entry.code_def(s, ct);
  }
}

// generate code to define a global string constant
void StringEntry::code_def(std::ostream &s, CgenClassTable *ct) {
#ifdef MP3
  // TODO: add code here
#endif
}

/*********************************************************************

  CgenNode methods

*********************************************************************/

//
// Class setup. You may need to add parameters to this function so that
// the classtable can provide setup information (such as the class tag
// that should be used by this class).
//
// Things that setup should do:
//  - layout the features of the class
//  - create the types for the class and its vtable
//  - create global definitions used by the class such as the class vtable
//
void CgenNode::setup(int tag, int depth) {
  this->tag = tag;
#ifdef MP3
  layout_features();

  // TODO: add code here

#endif
}

#ifdef MP3
// Laying out the features involves creating a Function for each method
// and assigning each attribute a slot in the class structure.
void CgenNode::layout_features() {
  // TODO: add code here
}

// Class codegen. This should performed after every class has been setup.
// Generate code for each method of the class.
void CgenNode::code_class() {
  // No code generation for basic classes. The runtime will handle that.
  if (basic()) {
    return;
  }
  // TODO: add code here
}

void CgenNode::code_init_function(CgenEnvironment *env) {
  // TODO: add code here
}

#else

// code-gen function main() in class Main
void CgenNode::codeGenMainmain() {
  // In Phase 1, this can only be class Main. Get method_class for main().
  assert(std::string(this->name->get_string()) == std::string("Main"));
  method_class *mainMethod = (method_class *)features->nth(features->first());

  // TODO: add code here to generate the function `int Mainmain()`.
  // Generally what you need to do are:
  ValuePrinter vp(*ct_stream);

  vp.define(*ct_stream, op_type(INT32), "Main_main", {});
  vp.begin_block("entry");

  // -- setup or create the environment, env, for translating this method
  // -- invoke mainMethod->code(env) to translate the method
  CgenEnvironment *env = new CgenEnvironment(*ct_stream, this);
  mainMethod->code(env); 

  vp.begin_block("abort");
  vp.call({}, op_type(VOID), "abort", true, {});
  vp.unreachable();

  vp.end_define();

  delete env;
}

#endif

/*********************************************************************

  CgenEnvironment functions

*********************************************************************/

// Look up a CgenNode given a symbol
CgenNode *CgenEnvironment::type_to_class(Symbol t) {
  return t == SELF_TYPE ? get_class()
                        : get_class()->get_classtable()->find_in_scopes(t);
}

/*********************************************************************

  APS class methods

    Fill in the following methods to produce code for the
    appropriate expression. You may add or remove parameters
    as you wish, but if you do, remember to change the parameters
    of the declarations in `cool-tree.handcode.h'.

*********************************************************************/

void program_class::cgen(const std::optional<std::string> &outfile) {
  initialize_constants();
  if (outfile) {
    std::ofstream s(*outfile);
    if (!s.good()) {
      std::cerr << "Cannot open output file " << *outfile << std::endl;
      exit(1);
    }
    class_table = new CgenClassTable(classes, s);
  } else {
    class_table = new CgenClassTable(classes, std::cout);
  }
}

// Create a method body
void method_class::code(CgenEnvironment *env) {
  if (cgen_debug) {
    std::cerr << "method" << std::endl;
  }

  ValuePrinter vp(*env->cur_stream);
  // TODO: add code here
  expr->make_alloca(env);
  operand main = expr->code(env);
  vp.ret(*env->cur_stream, main);
}

// Codegen for expressions. Note that each expression has a value.

operand assign_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "assign" << std::endl;

  ValuePrinter vp(*env->cur_stream);
  // TODO: add code here and replace `return operand()
  
  operand *left = env->find_in_scopes(name);
  operand right = expr->code(env);
  vp.store(*env->cur_stream, right, *left);
  return right;
}

operand cond_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "cond" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);

  operand cond_val = pred->code(env);

  std::string lthen = env->new_label("then", false);
  std::string lelse = env->new_label("else", false);
  std::string lend = env->new_label("end", true);

  vp.branch_cond(cond_val, lthen, lelse);

  vp.begin_block(lthen);
  operand then_val = then_exp->code(env);
  vp.store(*env->cur_stream, then_val, res_ptr);
  vp.branch_uncond(lend);

  vp.begin_block(lelse);
  operand else_val = else_exp->code(env);
  vp.store(*env->cur_stream, else_val, res_ptr);
  vp.branch_uncond(lend);

  vp.begin_block(lend);

  return vp.load(result_type, res_ptr);
}

operand loop_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "loop" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);

  std::string lstart = env->new_label("loop", false);
  std::string lbody = env->new_label("body", false);
  std::string lend = env->new_label("end", true);
  
  vp.branch_uncond(lstart);

  vp.begin_block(lstart);
  operand cond_val = pred->code(env);
  vp.branch_cond(*env->cur_stream, cond_val, lbody, lend);

  vp.begin_block(lbody);
  body->code(env);  
  vp.branch_uncond(lstart);

  vp.begin_block(lend);
  
  return int_value(0);
}

operand block_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "block" << std::endl;

  // TODO: add code here and replace `return operand()`
  operand block_res;
  for (int i = body->first(); body->more(i); i = body->next(i)) {
    auto expr = body->nth(i);
    block_res = expr->code(env);
  }
  return block_res;
}

operand let_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "let" << std::endl;

  // TODO: add code here and replace `return operand()`

  ValuePrinter vp(*env->cur_stream);

  operand init_op = init->code(env);

  if (init_op.is_empty()) { 
    if (id_type.get_id() == INT32) {
        vp.store(*env->cur_stream, int_value(0), id_op);
    } else if (id_type.get_id() == INT1) {
      vp.store(*env->cur_stream, bool_value(false, true), id_op);
    } else {
      vp.store(*env->cur_stream, null_value(id_type), id_op);
          }
    } else {  
      vp.store(*env->cur_stream, init_op, id_op);
    }
    env->open_scope();
    env->add_binding(identifier, &id_op);
    operand let_res = body->code(env);
    env->close_scope();
    return let_res;
}

operand plus_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "plus" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);
  operand right = e2->code(env);
  return vp.add(left, right);
}

operand sub_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "sub" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);
  operand right = e2->code(env);
  return vp.sub(left, right);
}

operand mul_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "mul" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);
  operand right = e2->code(env);
  return vp.mul(left, right);
}

operand divide_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "div" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);

  operand numerator = e1->code(env);
  operand demoninator = e2->code(env);
  operand zero = vp.icmp(EQ, demoninator, int_value(0));

  label safe_divide = env->new_ok_label();
  vp.branch_cond(*env->cur_stream, zero, "abort", safe_divide);
  vp.begin_block(safe_divide);
  return vp.div(numerator, demoninator);
}

operand neg_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "neg" << std::endl;

  // TODO: add code here and replace `return operand()`

  ValuePrinter vp(*env->cur_stream);
  operand right = e1->code(env);
  return vp.sub(int_value(0), right);
}

operand lt_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "lt" << std::endl;

  // TODO: add code here and replace `return operand()`

  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);
  operand right = e2->code(env);

  return vp.icmp(LT, left, right);
}

operand eq_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "eq" << std::endl;

  // TODO: add code here and replace `return operand()`

  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);
  operand right = e2->code(env);

  return vp.icmp(EQ, left, right);
}

operand leq_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "leq" << std::endl;

 // TODO: add code here and replace `return operand()`

  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);
  operand right = e2->code(env);

  return vp.icmp(LE, left, right);
}

operand comp_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "complement" << std::endl;

  // TODO: add code here and replace `return operand()`

  ValuePrinter vp(*env->cur_stream);
  operand left = e1->code(env);

  return vp.xor_in(left, bool_value(true, true));
}

operand int_const_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "Integer Constant" << std::endl;

  // TODO: add code here and replace `return operand()`
  ValuePrinter vp(*env->cur_stream);

  return int_value(atoi(token->get_string().c_str()));
}

operand bool_const_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "Boolean Constant" << std::endl;

  // TODO: add code here and replace `return operand()`
  if (val) {
    return bool_value(true, true);
  } 
  
  return bool_value(false, true);
}

operand object_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "Object" << std::endl;

  // TODO: add code here and replace `return operand()`
  // find
  ValuePrinter vp(*env->cur_stream);
  operand *object = env->find_in_scopes(name);
  op_type type = object->get_type().get_deref_type();
  operand object_res = vp.load(type, *object);

  return object_res;
}

operand no_expr_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "No_expr" << std::endl;

  // TODO: add code here and replace `return operand()`
  return operand();
}

//*****************************************************************
// The next few functions are for node types not supported in Phase 1
// but these functions must be defined because they are declared as
// methods via the Expression_SHARED_EXTRAS hack.
//*****************************************************************

operand static_dispatch_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "static dispatch" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

operand string_const_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "string_const" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

operand dispatch_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "dispatch" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

// Handle a Cool case expression (selecting based on the type of an object)
operand typcase_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "typecase::code()" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

operand new__class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "newClass" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

operand isvoid_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "isvoid" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

// Create the LLVM Function corresponding to this method.
void method_class::layout_feature(CgenNode *cls) {
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here
#endif
}

// Handle one branch of a Cool case expression.
// If the source tag is >= the branch tag
// and <= (max child of the branch class) tag,
// then the branch is a superclass of the source.
// See the MP3 handout for more information about our use of class tags.
operand branch_class::code(operand expr_val, operand tag, op_type join_type,
                           CgenEnvironment *env) {
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here and replace `return operand()`
  return operand();
#endif
}

// Assign this attribute a slot in the class structure
void attr_class::layout_feature(CgenNode *cls) {
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here
#endif
}

void attr_class::code(CgenEnvironment *env) {
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here
#endif
}

/*
 * Definitions of make_alloca
 */
void assign_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "assign" << std::endl;

  // TODO: add code here
  expr->make_alloca(env);
  set_type(env, expr->get_type(env));
}

void cond_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "cond" << std::endl;

  // TODO: add code here
  ValuePrinter vp(*env->cur_stream);

  pred->make_alloca(env);
  then_exp->make_alloca(env);
  else_exp->make_alloca(env);

  result_type = then_exp->get_type(env);
  res_ptr = vp.alloca_mem(result_type);
  set_type(env, result_type);
}

void loop_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "loop" << std::endl;

  // TODO: add code here
  ValuePrinter vp(*env->cur_stream);

  pred->make_alloca(env);
  body->make_alloca(env);

  set_type(env, op_type(INT32));
}

void block_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "block" << std::endl;

  // TODO: add code here
  for (int i = body->first(); body->more(i); i = body->next(i)) {
    auto expr = body->nth(i);
    expr->make_alloca(env);
  }
  if (body->len() > 0) {
    auto last_expr = body->nth(body->len() - 1);
    set_type(env, last_expr->get_type(env));
  } else {
    set_type(env, op_type(VOID));
  }
}

/* LLM Block 1 start*/
void let_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "let" << std::endl;

  // TODO: add code here
  init->make_alloca(env);

  ValuePrinter vp(*env->cur_stream);
  std::string name = type_decl->get_string();

  if (name == "Int") {
    id_type = op_type(INT32);
  } else if (name == "Bool") {
    id_type = op_type(INT1);
  } else {
    id_type = op_type(INT8_PTR);
  }

  id_op = vp.alloca_mem(id_type);

  env->type_open_scope();
  env->type_add_binding(identifier, &id_type);
  body->make_alloca(env);
  set_type(env, body->get_type(env));
  env->type_close_scope();
}
/* LLM Block 1 end*/

void plus_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "plus" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, e1->get_type(env));
}

void sub_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "sub" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, e1->get_type(env));
}

void mul_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "mul" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, e1->get_type(env));
}

void divide_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "div" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, e1->get_type(env));
}

void neg_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "neg" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  set_type(env, e1->get_type(env));
}

void lt_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "lt" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, op_type(INT1));
}

void eq_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "eq" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, op_type(INT1));
}

void leq_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "leq" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  e2->make_alloca(env);
  set_type(env, op_type(INT1));
}

void comp_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "complement" << std::endl;

  // TODO: add code here
  e1->make_alloca(env);
  set_type(env, e1->get_type(env));
}

void int_const_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "Integer Constant" << std::endl;

  // TODO: add code here
  set_type(env, op_type(INT32));
}

void bool_const_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "Boolean Constant" << std::endl;

  // TODO: add code here
  set_type(env, op_type(INT1));
}

void object_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "Object" << std::endl;

  // TODO: add code here
  set_type(env, *env->type_find_in_scopes(name));
}

void no_expr_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "No_expr" << std::endl;

  // TODO: add code here
  set_type(env, op_type());
}

void static_dispatch_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "static dispatch" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
    // TODO: add code here
#endif
}

void string_const_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "string_const" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
    // TODO: add code here
#endif
}

void dispatch_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "dispatch" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
    // TODO: add code here
#endif
}

void typcase_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "typecase::make_alloca()" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
    // TODO: add code here
#endif
}

void new__class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "newClass" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
    // TODO: add code here
#endif
}

void isvoid_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "isvoid" << std::endl;
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
    // TODO: add code here
#endif
}

void branch_class::make_alloca(CgenEnvironment *env) {
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here
#endif
}

void method_class::make_alloca(CgenEnvironment *env) { return; }

void attr_class::make_alloca(CgenEnvironment *env) {
#ifndef MP3
  assert(0 && "Unsupported case for phase 1");
#else
  // TODO: add code here
#endif
}

#ifdef MP3
// conform - If necessary, emit a bitcast or boxing/unboxing operations
// to convert an object to a new type. This can assume the object
// is known to be (dynamically) compatible with the target type.
// It should only be called when this condition holds.
// (It's needed by the supplied code for typecase)
operand conform(operand src, op_type type, CgenEnvironment *env) {
  // TODO: add code here
  return operand();
}
#endif

/* LLM Block 1 */
/* Tool use: Claude
  this is my question:
  these are my current let_class help me improve
operand let_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "let" << std::endl;

  // TODO: add code here and replace return operand()

  ValuePrinter vp(*env->cur_stream);

  operand init_op = init->code(env);

  if (init_op.is_empty()) { 
    if (id_type.get_id() == INT32) {
        vp.store(env->cur_stream, int_value(0), id_op);
    } else if (id_type.get_id() == INT1) {
      vp.store(env->cur_stream, bool_value(false, true), id_op);
    } else {
      vp.store(env->cur_stream, null_value(id_type), id_op);
          }
    } else {  
      vp.store(env->cur_stream, init_op, id_op);
    }
    env->open_scope();
    env->add_binding(identifier, &id_op);
    operand let_res = body->code(env);
    env->close_scope();
    return let_res;
}
void let_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "let" << std::endl;

  // TODO: add code here
  init->make_alloca(env);

  ValuePrinter vp(*env->cur_stream);
  std::string type_name = type_decl->get_string();

  env->open_scope();
  env->type__open_scope();

  if (type_name == "Int") {
    operand int = vp.alloca_mem(op_type(INT32));
    id_type = op_type(INT32);
    id_op = int;
    env->add_binding(identifier, &id_op);
    env->type_add_binding(identifier, &op_type(INT32));
  } else if (type_name == "Bool") {
    operand bool = vp.alloca_mem(op_type(INT1));
    id_type = op_type(INT1);
    id_op = bool;
    env->add_binding(identifier, &id_op);
    env->type_add_binding(identifier, &op_type(INT1));
  }

  body->make_alloca(env);
  set_expr_type(env, body->get_expr_type(env));

  env->close_scope();
  env->type_close_scope();
}
you may need to take a look at the parameters I defined in this cgen.h file
  void close_scope() { var_table.exitscope(); }

and this is the response
Your implementation of let_class is generally good, but there are a few improvements we can make to enhance its correctness and efficiency. Let's refine both the code and make_alloca methods:
cppCopyvoid let_class::make_alloca(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "let" << std::endl;

  init->make_alloca(env);

  ValuePrinter vp(*env->cur_stream);
  std::string type_name = type_decl->get_string();

  if (type_name == "Int") {
    id_type = op_type(INT32);
  } else if (type_name == "Bool") {
    id_type = op_type(INT1);
  } else {
    // Handle other types (for MP3)
    id_type = op_type(INT8_PTR);
  }

  id_op = vp.alloca_mem(id_type);

  // Process the body in the new scope
  env->type_open_scope();
  env->type_add_binding(identifier, &id_type);
  body->make_alloca(env);
  set_expr_type(env, body->get_expr_type(env));
  env->type_close_scope();
}

operand let_class::code(CgenEnvironment *env) {
  if (cgen_debug)
    std::cerr << "let" << std::endl;

  ValuePrinter vp(*env->cur_stream);

  operand init_op = init->code(env);

  if (init_op.is_empty()) { 
    if (id_type.get_id() == INT32) {
      vp.store(*env->cur_stream, int_value(0), id_op);
    } else if (id_type.get_id() == INT1) {
      vp.store(*env->cur_stream, bool_value(false, true), id_op);
    } else {
      vp.store(*env->cur_stream, null_value(id_type), id_op);
    }
  } else {  
    vp.store(*env->cur_stream, init_op, id_op);
  }

  env->open_scope();
  env->add_binding(identifier, &id_op);
  operand let_res = body->code(env);
  env->close_scope();

  return let_res;
}
Key improvements and explanations:

In make_alloca:

We've moved the scope opening to just before processing the body, as the let variable should not be in scope for its own initialization.
We've simplified the type assignment and allocation.
We're now only opening/closing the var_tp_scope, not the regular scope.


In code:

The structure remains largely the same, which is good.
We're correctly handling the initialization and scoping.


General improvements:

We've added a case for handling other types (using INT8_PTR as a generic pointer type), which will be useful for MP3.
The code is now more consistent between make_alloca and code.


Regarding the CgenEnvironment:

Your implementation correctly uses open_scope() and close_scope() for the variable table.
It also correctly uses var_tp_open_scope() and var_tp_close_scope() for the type table.



These changes should make your let_class implementation more robust and prepare it for the more complex cases you'll encounter in MP3. The separation of concerns between make_alloca (which handles type setup and memory allocation) and code (which handles actual code generation) is maintained and improved.
 */