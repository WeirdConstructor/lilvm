// Copyright (C) 2017 Weird Constructor
// For more license info refer to the the bottom of this file.

#include "buklivm.h"
#include "atom_printer.h"
#include "atom_cpp_serializer.h"
#include "util.h"
#include "modules/bklisp_module_wrapper.h"
#include <chrono>

using namespace std;

namespace bukalisp
{
//---------------------------------------------------------------------------

std::string PROG::as_string()
{
    std::stringstream ss;
    ss << "#<prog:\n";
    for (size_t i = 0; i < m_atom_data.m_d.vec->m_len; i++)
    {
        ss << " [" << i << "]: " << write_atom(m_atom_data.m_d.vec->m_data[i]) << "\n";
    }

    for (size_t i = 0; i < m_instructions_len; i++)
    {
        ss << " {" << i << "}: ";
        m_instructions[i].to_string(ss);
        ss << "\n";
    }
    ss << ">";
    return ss.str();
}
//---------------------------------------------------------------------------

void parse_op_desc(INST &instr, Atom &desc)
{
    if (desc.m_type != T_VEC)
        return;

    AtomVec *av = desc.m_d.vec;
    if (av->m_len < 7)
    {
        cout << "Too few elements in op desc (7 needed): "
             << desc.to_write_str() << endl;
        return;
    }

    std::string op_name;
    Atom op_name_s = av->m_data[0];
    if (   op_name_s.m_type != T_SYM
        && op_name_s.m_type != T_KW)
        return;

    op_name = op_name_s.m_d.sym->m_str;

    uint8_t op_code = 0;

#define X(name, code) else if (op_name == #name) op_code = code;
    if (op_name == "***") op_code = 0;
    OP_CODE_DEF(X)
    else
    {
        cout << "unknown op name: " << op_name << endl;
    }
#undef X

    instr.op = op_code;
    instr.o  = (int32_t) av->m_data[1].to_int();
    instr.oe = (int32_t) av->m_data[2].to_int();
    instr.a  = (int32_t) av->m_data[3].to_int();
    instr.ae = (int32_t) av->m_data[4].to_int();
    instr.b  = (int32_t) av->m_data[5].to_int();
    instr.be = (int32_t) av->m_data[6].to_int();
    instr.c  = (int32_t) av->m_data[7].to_int();
    instr.ce = (int32_t) av->m_data[8].to_int();
}
//---------------------------------------------------------------------------

Atom make_prog(GC &gc, Atom prog_info)
{
    if (prog_info.m_type != T_VEC)
        return Atom();

    Atom data     = prog_info.m_d.vec->m_data[0];
    Atom prog     = prog_info.m_d.vec->m_data[1];
    Atom debug    = prog_info.m_d.vec->m_data[2];

    if (   data.m_type      != T_VEC
        || prog.m_type      != T_VEC
        || debug.m_type     != T_MAP)
        return Atom();

    PROG *new_prog =
        new PROG(gc,
                 data.m_d.vec->m_len,
                 prog.m_d.vec->m_len + 1);
    new_prog->set_debug_info(debug);
    new_prog->set_data_from(data.m_d.vec);

    for (size_t i = 0; i < prog.m_d.vec->m_len + 1; i++)
    {
        INST instr;
        if (i == (prog.m_d.vec->m_len))
        {
            instr.op = OP_END;
            instr.o  = 0;
            instr.oe = 0;
            instr.a  = 0;
            instr.ae = 0;
            instr.b  = 0;
            instr.be = 0;
            instr.c  = 0;
            instr.ce = 0;
        }
        else
            parse_op_desc(instr, prog.m_d.vec->m_data[i]);
        new_prog->set(i, instr);
    }

    Atom ud(T_UD);
    ud.m_d.ud = new_prog;
    Atom newud = ud;
    return ud;
}
//---------------------------------------------------------------------------

void VM::init_prims()
{
    Atom tmp;

#define START_PRIM() \
    tmp = Atom(T_PRIM); \
    tmp.m_d.func = new Atom::PrimFunc; \
    m_prim_table->push(tmp); \
    (*tmp.m_d.func) = [this](AtomVec &args, Atom &out) {

#define END_PRIM(name) \
    }; \
    m_prim_sym_table->push(Atom(T_SYM, m_rt->m_gc.new_symbol(#name)));

#define END_PRIM_DOC(name, docstr) \
    }; \
    m_prim_sym_table->push(Atom(T_SYM, m_rt->m_gc.new_symbol(#name))); \
    m_vm->set_documentation(#name, docstr);

    #include "primitives.cpp"
}
//---------------------------------------------------------------------------

void VM::load_module(BukaLISPModule *m)
{
    Atom funcs(T_MAP, m_rt->m_gc.allocate_map());
    m_modules->set(m->module_name(this), funcs);

    Atom init_func;
    for (size_t i = 0; i < m->function_count(); i++)
    {
        Atom func_desc(T_VEC, m_rt->m_gc.allocate_vector(3));
        func_desc.m_d.vec->push(m->get_func_name(this, i));
        func_desc.m_d.vec->push(Atom(T_INT, m_prim_table->m_len));
        func_desc.m_d.vec->push(m->get_func(this, i));
        funcs.m_d.map->set(func_desc.at(0), func_desc);

        m_prim_table->push(func_desc.at(2));

        if (func_desc.at(0).m_d.sym->m_str == "__INIT__")
            init_func = func_desc.at(2);
    }

    if (init_func.m_type == T_PRIM)
    {
        GC_ROOT_VEC(m_rt->m_gc, args) = m_rt->m_gc.allocate_vector(0);
        Atom ret;
        (*init_func.m_d.func)(*args, ret);
    }
    else if (init_func.m_type != T_NIL)
    {
        throw VMException(
                "Bad __INIT__ in module initialization of '"
                + m->module_name(this).m_d.sym->m_str
                + "'");
    }
}
//---------------------------------------------------------------------------

AtomMap *VM::loaded_modules()
{
    return m_modules;
}
//---------------------------------------------------------------------------

#define CHECK_ARITY(arity, argc)             \
    ((((arity).m_type == T_NIL)              \
       || ((arity).m_d.i == (argc))) ? 0     \
    : ((arity).m_d.i < 0)                    \
    ? (((argc) < -((arity).m_d.i)) ? -1 : 0) \
    : ((arity).m_d.i > (argc)) ? -1          \
    : ((arity).m_d.i < (argc)) ? 1           \
    : 0)
//---------------------------------------------------------------------------

void VM::report_arity_error(Atom &arity, size_t argc)
{
    int err = CHECK_ARITY(arity, argc);
    int exp = (int) abs(arity.m_d.i);
    if (err < 0)
    {
        error("lambda call with too few arguments, expected "
              + to_string(exp) + " but got",
              Atom(T_INT, argc));
    }
    else if (err > 0)
    {
        error("lambda call with too many arguments, expected "
              + to_string(exp) + " but got",
              Atom(T_INT, argc));
    }
}
//---------------------------------------------------------------------------

Atom VM::eval(Atom callable, AtomVec *root_env, AtomVec *args)
{
    using namespace std::chrono;

//(define REG-ROW-FRAME 0)
//(define REG-ROW-DATA  1)
//(define REG-ROW-PRIM  2)
//(define REG-ROW-UPV   3)
//(define REG-ROW-ROOT  4)
#define REG_ROW_FRAME   0
#define REG_ROW_DATA    1
#define REG_ROW_PRIM    2
#define REG_ROW_UPV     3
#define REG_ROW_ROOT    4
#define REG_ROWS        5

    GC_ROOT_VEC(m_rt->m_gc, keep_alive_reg_rows) =
        m_rt->m_gc.allocate_vector(2);
    keep_alive_reg_rows->check_size(1);
    AtomVec *reg_row[REG_ROWS];
#define SET_FRAME_ROW(row_ptr)  reg_row[REG_ROW_FRAME] = row_ptr; keep_alive_reg_rows->m_data[0].set_vec(reg_row[REG_ROW_FRAME]);
#define SET_DATA_ROW(row_ptr)   reg_row[REG_ROW_DATA]  = row_ptr;
#define SET_PRIM_ROW(row_ptr)   reg_row[REG_ROW_PRIM]  = row_ptr;
#define SET_UPV_ROW(row_ptr)    reg_row[REG_ROW_UPV]   = row_ptr; keep_alive_reg_rows->m_data[1].set_vec(reg_row[REG_ROW_UPV]);
#define SET_ROOT_ROW(row_ptr)   reg_row[REG_ROW_ROOT]  = row_ptr;

    PROG *prog = nullptr;
    INST *pc   = nullptr;

    if (!args) args = m_rt->m_gc.allocate_vector(0);

    GC_ROOT_VEC(m_rt->m_gc, args_root)     = args;
    GC_ROOT_VEC(m_rt->m_gc, cont_stack)    = nullptr;
    GC_ROOT_VEC(m_rt->m_gc, empty_upv_row) = m_rt->m_gc.allocate_vector(0);
    AtomVec *clos_upvalues = empty_upv_row;

    if (callable.m_type == T_UD && callable.m_d.ud->type() == "VM-PROG")
    {
        prog      = dynamic_cast<PROG*>(callable.m_d.ud);
        pc        = &(prog->m_instructions[0]);
    }
    else if (callable.m_type == T_CLOS)
    {
        if (   callable.m_d.vec->m_len == 4
            && callable.m_d.vec->m_data[0].m_type == T_UD)
        {
            prog = dynamic_cast<PROG*>(callable.m_d.vec->m_data[0].m_d.ud);
            pc   = &(prog->m_instructions[0]);
            if (CHECK_ARITY(callable.m_d.vec->m_data[3], args->m_len) != 0)
                report_arity_error(callable.m_d.vec->m_data[3], args->m_len);
            if (callable.m_d.vec->m_data[1].m_d.vec)
                clos_upvalues = callable.m_d.vec->m_data[1].m_d.vec;
            // TODO: Handle calling a coroutine!
        }
        else if (callable.m_d.vec->m_len == 3)
        {
            if (!m_interpreter_call)
            {
                cout << "VM-ERROR: Can't call eval from interpreter in VM. "
                        "No interpreter?" << endl;
                return Atom();
            }
            return m_interpreter_call(callable, args);
        }
        else
        {
            cout << "VM-ERROR: Broken closure input to run-vm-prog! "
                 << callable.to_write_str() << endl;
            return Atom();
        }
    }
    else if (callable.m_type == T_PRIM)
    {
        Atom ret;
        (*callable.m_d.func)(*args, ret);
        return ret;
    }
    else
    {
//        error("Bad input type to run-vm-prog, expected VM-PROG.", A0);
        cout << "VM-ERROR: Bad input type to run-vm-prog, expected VM-PROG or closure, got: "
             << callable.to_write_str() << endl;
        return Atom();
    }

    cout << "vm start" << endl;

    VMProgStateGuard psg(m_prog, m_pc, prog, pc);

    SET_ROOT_ROW(root_env);
    SET_DATA_ROW(m_prog->data_array());
    SET_UPV_ROW(clos_upvalues);
    SET_PRIM_ROW(m_prim_table);
    SET_FRAME_ROW(args);

    // TODO: Check:
    // XXX: The root-call-frame actually keeps alive the whole code-tree, including
    //      any callable sub-m_prog objects. Thus, we don't have to explicitly
    //      keep alive the m-prog or it's data_array().
    //
    //      => We just need to keep alive the UPV_ROW and the FRAME_ROW.
    cont_stack = m_rt->m_gc.allocate_vector(0);
    {
        AtomVec *call_frame = m_rt->m_gc.allocate_vector(7);
        call_frame->set(0, callable);
        call_frame->set(6, Atom());
        cont_stack->push(Atom(T_VEC, call_frame));
    }

//    cout << "VM PROG: " << callable.to_write_str() << endl;

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    Atom static_nil_atom;
#define P_A  ((PC.a))
#define P_B  ((PC.b))
#define P_C  ((PC.c))
#define P_O  ((PC.o))
#define PE_A ((PC.ae))
#define PE_B ((PC.be))
#define PE_C ((PC.ce))
#define PE_O ((PC.oe))

#define E_SET_CHECK_REALLOC_D(env_idx, idx) do { \
    if ((env_idx) < REG_ROWS) \
    { \
        reg_row[(env_idx)]->check_size((idx)); \
    } \
    else \
        error("out of reg_row range (" #env_idx ")", Atom(T_INT, (env_idx))); \
} while (0)
#define E_SET_CHECK_REALLOC(env_idx_reg, idx_reg) \
    E_SET_CHECK_REALLOC_D(PE_##env_idx_reg, P_##idx_reg)

#define E_SET_D_PTR(env_idx, idx, val_ptr) do { \
    val_ptr = &(reg_row[(env_idx)]->m_data[(idx)]); \
    if (env_idx == REG_ROW_UPV) val_ptr = &(val_ptr->m_d.vec->m_data[0]); \
} while (0)

#define E_SET_D(env_idx, idx, val) do { \
    if (env_idx != REG_ROW_UPV) \
        reg_row[(env_idx)]->m_data[(idx)] = (val); \
    else \
        reg_row[(env_idx)]->m_data[(idx)].m_d.vec->m_data[0] = (val); \
} while (0)

#define E_SET(reg, val) E_SET_D(PE_##reg, P_##reg, (val))

#define E_GET_D(out_ptr, eidx, idx) do { \
    if ((idx) >= (int32_t) reg_row[(eidx)]->m_len) \
        (out_ptr) = &static_nil_atom; \
    else { \
        (out_ptr) = &(reg_row[(eidx)]->m_data[(idx)]); \
        if ((eidx) == REG_ROW_UPV) \
        { \
            if (out_ptr->m_type != T_VEC || out_ptr->m_d.vec->m_len <= 0) \
                error("Bad upvalue access", Atom(T_INT, (idx))); \
            (out_ptr) = &(out_ptr->m_d.vec->m_data[0]); \
        } \
    } \
} while (0)

#define E_GET(out_ptr, reg) E_GET_D(out_ptr, PE_##reg, P_##reg)

// TODO: Check:
#define RESTORE_FROM_CALL_FRAME(call_frame, ret_val)  \
    do {                                              \
        Atom *cv = (call_frame).m_d.vec->m_data;        \
        if (cv[1].m_type == T_NIL)                    \
        {                                             \
            ret = (ret_val);                          \
            m_pc =                                    \
                &(m_prog->m_instructions[             \
                    m_prog->m_instructions_len - 2]); \
            break;                                    \
        }                                             \
        Atom proc    = cv[1];                         \
        Atom upvs    = cv[2];                         \
        Atom frame   = cv[3];                         \
        Atom pc      = cv[4];                         \
        int32_t oidx = (int32_t) cv[5].m_d.i;         \
        int32_t eidx = (int32_t) cv[6].m_d.i;         \
                                                      \
        m_prog   = static_cast<PROG*>(proc.m_d.ud);   \
        m_pc     = (INST *) pc.m_d.ptr;               \
        SET_DATA_ROW(m_prog->data_array());           \
        SET_UPV_ROW(upvs.m_d.vec);                    \
        SET_FRAME_ROW(frame.m_d.vec);                 \
                                                      \
        E_SET_CHECK_REALLOC_D(eidx, oidx);            \
        E_SET_D(eidx, oidx, (ret_val));               \
        if (m_trace)                                  \
            cout << "RETURN(" << oidx << ":" << eidx  \
                 << ") => "                           \
                 << (ret_val).to_write_str() << endl; \
    } while(0);

#define RECORD_CALL_FRAME(func, call_frame)             \
    Atom call_frame(T_VEC,                              \
        m_rt->m_gc.allocate_vector(7));                 \
    do {                                                \
        AtomVec *cv = call_frame.m_d.vec;               \
        cv->m_len = 7;                                  \
        Atom *data = cv->m_data;                        \
                                                        \
        data[0].set_clos(func.m_d.vec);                 \
        data[1].set_ud((UserData *) m_prog);            \
        data[2].set_vec(reg_row[REG_ROW_UPV]);          \
        data[3].set_vec(reg_row[REG_ROW_FRAME]);        \
        data[4].set_ptr(m_pc);                          \
        data[5].set_int(P_O);                           \
        data[6].set_int(PE_O);                          \
    } while(0)

    Atom ret;
    Atom *tmp = nullptr;

    bool alloc = false;
    try
    {
        while (m_pc->op != OP_END)
        {
            INST &PC = *m_pc;

            if (m_trace)
            {
                cout << "VMTRC FRMS(" << cont_stack->m_len << "): ";
                m_pc->to_string(cout);
                cout << "ROOT: (" << Atom(T_VEC, reg_row[REG_ROW_ROOT]).to_write_str() << ")";
                cout << " {ENV= ";
                for (size_t i = 0; i < reg_row[REG_ROW_FRAME]->m_len; i++)
                {
                    Atom &a = reg_row[REG_ROW_FRAME]->m_data[i];
                    if (a.m_type == T_UD && a.m_d.ud->type() == "VM-PROG")
                        cout << i << "[#<prog>] ";
                    else
                        cout << i << "[" << a.to_write_str() << "] ";
                }
                cout << "}" << get_current_debug_info().to_write_str() << endl;
            }

            switch (m_pc->op)
            {
                case OP_DUMP_ENV_STACK:
                {
                // TODO: Check
//                    for (size_t i = 0; i < frm_stack->m_len; i++)
//                    {
//                        cout << "ENV@" << (frm_stack->m_len - (i + 1)) << ": ";
//                        AtomVec *env = frm_stack->at(i).m_d.vec;
//                        for (size_t j = 0; j < env->m_len; j++)
//                            cout << "[" << j << "=" << env->at(j).to_write_str() << "] ";
//                        cout << endl;
//                    }
                    break;
                }

                case OP_MOV:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    E_GET(tmp, A);
                    E_SET(O, *tmp);
                    break;
                }

                case OP_NEW_ARG_VEC:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    alloc = true;

                    AtomVec *out_av = m_rt->m_gc.allocate_vector(P_A);
                    E_SET(O, Atom(T_VEC, out_av));

                    E_GET(tmp, B);
                    if (tmp->m_type != T_VEC)
                        error("Bad argument index vector!", *tmp);
                    AtomVec *av = tmp->m_d.vec;

                    for (size_t i = 0; i < av->m_len; i += 2)
                    {
                        Atom *tmp_idx  = &(av->m_data[i]);
                        Atom *tmp_eidx = &(av->m_data[i + 1]);
                        E_GET_D(tmp, tmp_eidx->m_d.i, tmp_idx->m_d.i);
                        out_av->push(*tmp);
                    }

                    break;
                }

                case OP_NEW_VEC:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    alloc = true;
                    E_SET(O, Atom(T_VEC, m_rt->m_gc.allocate_vector(P_A)));
                    break;
                }

                case OP_NEW_MAP:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    alloc = true;
                    E_SET(O, Atom(T_MAP, m_rt->m_gc.allocate_map()));
                    break;
                }

                case OP_CSET_VEC:
                {
                    E_GET(tmp, O);
                    Atom &vec = *tmp;

                    E_GET(tmp, B);
                    if (vec.m_type == T_VEC)
                    {
                        // XXX: Attention, *tmp is taken as reference and assigned
                        //      to a vector it is itself part of. In this case, the
                        //      vector might get reallocated and the *tmp reference
                        //      might be invalidated!
                        //      However, as *tmp comes from the current environment
                        //      frame, this should not happen, until we get
                        //      direct access to the environment/frame vector.
                        vec.m_d.vec->set(P_A, *tmp);
                    }
                    else if (vec.m_type == T_MAP)
                    {
                        Atom *key;
                        E_GET(key, A);
                        vec.m_d.map->set(*key, *tmp);
                    }
                    else
                        error("Can CSET_VEC on vector and map", vec);

                    break;
                }

                case OP_SET:
                {
                    E_GET(tmp, O);
                    Atom &vec = *tmp;
                    Atom *key;
                    E_GET(key, A);

                    E_GET(tmp, B);
                    if (vec.m_type == T_VEC)
                    {
                        // XXX: Attention, *tmp is taken as reference and assigned
                        //      to a vector it is itself part of. In this case, the
                        //      vector might get reallocated and the *tmp reference
                        //      might be invalidated!
                        //      However, as *tmp comes from the current environment
                        //      frame, this should not happen, until we get
                        //      direct access to the environment/frame vector.
                        vec.m_d.vec->set((size_t) key->to_int(), *tmp);
                    }
                    else if (vec.m_type == T_MAP)
                    {
                        vec.m_d.map->set(*key, *tmp);
                    }
                    else
                        error("Can SET on vector and map", vec);

                    break;
                }

                case OP_GET:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom &vec = *tmp;

                    Atom *key;
                    E_GET(key, B);

                    if (vec.m_type == T_VEC)
                    {
                        E_SET(O, vec.m_d.vec->at((size_t) key->to_int()));
                    }
                    else if (vec.m_type == T_MAP)
                    {
                        E_SET(O, vec.m_d.map->at(*key));
                    }
                    else
                        error("Can GET on vector and map", vec);

                    break;
                }

                case OP_LOAD_NIL:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    E_SET(O, Atom());
                    break;
                }

                case OP_NEW_CLOSURE:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    alloc = true;

                    E_GET(tmp, A);
                    Atom &prog = *tmp;
                    if (prog.m_type != T_UD || prog.m_d.ud->type() != "VM-PROG")
                        error("VM can't make closure from non VM-PROG", prog);

                    E_GET(tmp, B);
                    if (tmp->m_type != T_VEC)
                        error("Can't create closure without info", *tmp);
                    bool is_coroutine = !(tmp->at(0).is_false());

                    AtomVec *upvalues = nullptr;
                    if (tmp->at(2).m_type == T_VEC)
                    {
                        AtomVec *clos_upv_idxes = tmp->at(2).m_d.vec;
                        upvalues =
                            m_rt->m_gc.allocate_vector(clos_upv_idxes->m_len);
                        for (size_t i = 0; i < clos_upv_idxes->m_len; i++)
                        {
                            AtomVec *upv_reg_row = reg_row[REG_ROW_UPV];
                            size_t upv_idx =
                                (size_t) clos_upv_idxes->m_data[i].m_d.i;
                            if (upv_idx >= upv_reg_row->m_len)
                                error("Bad upvalue index", Atom(T_INT, upv_idx));
                            upvalues->set(i, upv_reg_row->m_data[upv_idx]);
                        }
                    }

    //                std::cout << "NEW CLOS " << ((void *) cl.m_d.vec) << "@" << ((void *) cl.m_d.vec->m_data) << std::endl;

                    Atom cl(T_CLOS, m_rt->m_gc.allocate_vector(4));
                    cl.m_d.vec->m_len = 4;
                    cl.m_d.vec->m_data[0].set_ud(prog.m_d.ud);
                    cl.m_d.vec->m_data[1].set_vec(upvalues);
                    cl.m_d.vec->m_data[2].set_bool(is_coroutine);
                    cl.m_d.vec->m_data[3] = tmp->at(1);

                    E_SET(O, cl);
                    break;
                }

                case OP_NEW_UPV:
                {
                    E_GET(tmp, A);
                    Atom &val = *tmp;
                    if (PE_O != REG_ROW_UPV)
                        error("Can't set upvalue on non upvalue register row",
                              Atom(T_INT, PE_O));

                    Atom upv(T_VEC, m_rt->m_gc.allocate_vector(1));
                    upv.m_d.vec->set(0, val);
                    reg_row[REG_ROW_UPV]->set(P_O, upv);
                    break;
                }

                case OP_PACK_VA:
                {
                    size_t pack_idx = P_A;
                    AtomVec *frame = reg_row[REG_ROW_FRAME];
                    if (pack_idx < frame->m_len)
                    {
                        size_t va_len = frame->m_len - pack_idx;
                        AtomVec *v = m_rt->m_gc.allocate_vector(va_len);
                        for (size_t i = 0; i < va_len; i++)
                            v->m_data[i] = frame->m_data[i + pack_idx];
                        v->m_len = va_len;
                        Atom vv(T_VEC, v);
                        E_SET_CHECK_REALLOC(O, O);
                        E_SET(O, vv);
                    }
                    else
                    {
                        E_SET_CHECK_REALLOC(O, O);
                        E_SET(O, Atom(T_VEC, m_rt->m_gc.allocate_vector(0)));
                    }
                    break;
                }

                case OP_CALL:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom &func = *tmp;
                    E_GET(tmp, B);
                    Atom &argv = *tmp;

                    if (argv.m_type != T_VEC)
                        error("CALL argv not a vector", argv);

                    if (func.m_type == T_PRIM)
                    {
                        Atom *ot;
                        E_SET_D_PTR(PE_O, P_O, ot);
                        (*func.m_d.func)(*(argv.m_d.vec), *ot);
    //                    cout << "argv: " << argv.to_write_str() << "; RET PRIM: "  << ret.to_write_str() << endl;
                        if (m_trace) cout << "CALL=> " << ot->to_write_str() << endl;

    //                    m_rt->m_gc.give_back_vector(argv.m_d.vec, true);
                    }
                    else if (func.m_type == T_CLOS)
                    {
                        if (func.m_d.vec->m_len < 4)
                            error("Bad closure found", func);
                        if (func.m_d.vec->m_data[0].m_type != T_UD)
                            error("Bad closure found", func);

                        if (CHECK_ARITY(func.m_d.vec->m_data[3],
                                        argv.m_d.vec->m_len) != 0)
                            report_arity_error(func.m_d.vec->m_data[3],
                                               argv.m_d.vec->m_len);

                        alloc = true;
                        // save the current execution context:
                        RECORD_CALL_FRAME(func, call_frame);
                        cont_stack->push(call_frame);

                        if (func.m_d.vec->m_data[2].m_type == T_VEC)
                        {
                            // XXX: Check if T_CLOS is a coroutine (!func.at(2).is_false())
                            //      Then instead of a new entry point in PROG, we restore
                            //      the former execution context by restoring the
                            //      call frames and the execution context
                            //      (m_prog, m_pc, frm_stack, output-register)
                            //      We also need to set the output-register to the
                            //      value of argv.at(0).

                            Atom ret_val = argv.at(0);

                            AtomVec *coro_cont_stack =
                                func.m_d.vec->m_data[2].m_d.vec;
                            func.m_d.vec->m_data[2].set_bool(true);

                            Atom restore_frame = *(coro_cont_stack->last());
                            coro_cont_stack->pop();
                            for (size_t i = 0; i < coro_cont_stack->m_len; i++)
                                cont_stack->push(coro_cont_stack->m_data[i]);

                            RESTORE_FROM_CALL_FRAME(restore_frame, ret_val);
                        }
                        else
                        {
                            m_prog   = dynamic_cast<PROG*>(func.m_d.vec->m_data[0].m_d.ud);
                            m_pc     = &(m_prog->m_instructions[0]);
                            m_pc--;

                            SET_DATA_ROW(m_prog->data_array());
                            SET_UPV_ROW(
                                m_rt->m_gc.clone_vector(
                                    func.m_d.vec->m_data[1].m_d.vec));
                            SET_FRAME_ROW(argv.m_d.vec);
                        }
                    }
                    else
                        error("CALL does not support that function type yet", func);

                    break;
                }

                case OP_RETURN:
                {

                    // We get the returnvalue here, so we don't get corrupt
                    // addresses when the later E_SET_D is maybe reallocating
                    // some address space we might be refering to.
                    Atom ret_val;
                    E_GET(tmp, O);
                    ret_val = *tmp;

                    // retrieve the continuation:
                    Atom *c = cont_stack->last();
                    if (!c || c->m_type == T_NIL)
                    {
                        ret = ret_val;
                        m_pc = &(m_prog->m_instructions[m_prog->m_instructions_len - 2]);
                        break;
                    }

                    Atom call_frame = *c;
                    cont_stack->pop();
                    if (call_frame.m_type != T_VEC || call_frame.m_d.vec->m_len < 4)
                        error("Empty or bad call frame stack item!", call_frame);

                    Atom *cv = call_frame.m_d.vec->m_data;
                    if (cv[1].m_type == T_NIL)
                    {
                        ret = ret_val;
                        m_pc = &(m_prog->m_instructions[m_prog->m_instructions_len - 2]);
                        break;
                    }

                    RESTORE_FROM_CALL_FRAME(call_frame, ret_val);
                    break;
                }

//                case OP_YIELD:
//                {
//                    E_GET(tmp, A);
//                    Atom ret_val = *tmp;
//
//                    Atom cur_func = cont_stack->last()->at(0);
//                    RECORD_CALL_FRAME(cur_func, coro_cont_frame);
//
//                    AtomVec *cont_copy =
//                        m_rt->m_gc.allocate_vector(cont_stack->m_len);
//
//                    // We need to copy all call frames except the one
//                    // with the coroutine in it (as that is the one,
//                    // where we need to return to now).
//                    // Additionally we need to record a "return point"
//                    // which we point to when recalling the coroutine.
//                    // This might be inside the call of a completely
//                    // other closure in fact.
//
//                    // =>
//                    // 1. record current program state (m_prog, m_pc, frm_stack)
//                    //    (current T_CLOS is part of the call frame on top of cont_stack)
//                    // 1b. save the O register where to put the continuation-value
//                    //     with the program state!
//                    // 2. copy all cont_stack frames except the one with the coroutine in it.
//                    // 3. save the prog-state and the cont_stack in the T_CLOS of
//                    //    the coroutine.
//                    // 4. return from the T_CLOS call as "usual" with the value
//                    //    passed into YIELD as 'A'.
//                    Atom ret_call_frame;
//                    Atom func;
//                    size_t ret_cont_stack_idx = 0;
//                    for (size_t i = cont_stack->m_len; i > 0; i--)
//                    {
//                        Atom &frm = cont_stack->m_data[i - 1];
//                        //d// cout << "FRM: " << frm.m_type << " F: " << frm.at(0).at(2).to_write_str() << endl;
//                        if (!frm.at(0).at(2).is_false())
//                        {
//                            //d// cout << "FOUND COROUTINE CALL AT DEPTH="
//                            //d//      << i << " of " << cont_stack->m_len << endl;
//                            ret_call_frame = frm;
//                            func           = frm.at(0);
//                            ret_cont_stack_idx = i - 1;
//                            break;
//                        }
//
//                        cont_copy->set((cont_stack->m_len - i), frm);
//                    }
//
//                    if (func.m_type == T_CLOS)
//                    {
//                        cont_copy->push(coro_cont_frame);
//                        func.m_d.vec->set(2, Atom(T_VEC, cont_copy));
//                    }
//                    else
//                        error("Can't 'yield' from a non-coroutine call!", ret_val);
//
//                    cont_stack->m_len = ret_cont_stack_idx;
//                    RESTORE_FROM_CALL_FRAME(ret_call_frame, ret_val);
//                    break;
//                }

                case OP_BR:
                {
                    m_pc += P_O;
                    if (m_pc >= (m_prog->m_instructions + m_prog->m_instructions_len))
                        error("BR out of PROG", Atom());
                    break;
                }

                case OP_BRIF:
                {
                    E_GET(tmp, A);
                    if (!tmp->is_false())
                    {
                        m_pc += P_O;
                        if (m_pc >= (m_prog->m_instructions + m_prog->m_instructions_len))
                            error("BRIF out of PROG", Atom());
                    }
                    break;
                }

                case OP_BRNIF:
                {
                    E_GET(tmp, A);
                    if (tmp->is_false())
                    {
                        m_pc += P_O;
                        if (m_pc >= (m_prog->m_instructions + m_prog->m_instructions_len))
                            error("BRIF out of PROG", Atom());
                    }
                    break;
                }

                case OP_FORINC:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    size_t idx_o = P_O + 1;
                    if (idx_o >= reg_row[REG_ROW_FRAME]->m_len)
                        reg_row[REG_ROW_FRAME]->set(idx_o, Atom(T_INT));

                    Atom *step;
                    E_GET(step, A);
                    E_GET(tmp, B);

                    Atom *ot;
                    E_GET(ot, C);
                    Atom &o = *ot;

                    Atom *condt;
                    E_SET_D_PTR(PE_O, P_O, condt);
                    Atom &cond = *condt;

                    bool update = cond.m_type == T_BOOL;

                    if (o.m_type == T_DBL)
                    {
                        double step_i = step->to_dbl();
                        double i = o.m_d.d;
                        if (update) i += step_i;
                        cond.set_bool(
                            step_i > 0.0
                            ? i > tmp->to_dbl()
                            : i < tmp->to_dbl());
                        o.m_d.d = i;
                    }
                    else
                    {
                        int64_t step_i = step->to_int();
                        int64_t i = o.m_d.i;
                        if (update) i += step_i;
                        cond.set_bool(
                            step_i > 0
                            ? i > tmp->to_int()
                            : i < tmp->to_int());
                        o.m_d.i = i;
                    }

                    break;
                }

                case OP_NOT:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom o(T_BOOL);
                    o.m_d.b = tmp->is_false();
                    E_SET(O, o);
                    break;
                }

                case OP_EQ:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom &a = *tmp;
                    E_GET(tmp, B);
                    Atom &b = *tmp;

                    Atom o(T_BOOL);
                    if (a.m_type == T_DBL)
                        o.m_d.b = std::fabs(a.to_dbl() - b.to_dbl())
                                  < std::numeric_limits<double>::epsilon();
                    else
                        o.m_d.b = a.to_int() == b.to_int();
                    E_SET(O, o);
                    break;
                }

                case OP_NEQ:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom &a = *tmp;
                    E_GET(tmp, B);
                    Atom &b = *tmp;

                    Atom o(T_BOOL);
                    if (a.m_type == T_DBL)
                        o.m_d.b = std::fabs(a.to_dbl() - b.to_dbl())
                                  >= std::numeric_limits<double>::epsilon();
                    else
                        o.m_d.b = a.to_int() != b.to_int();
                    E_SET(O, o);
                    break;
                }

                case OP_ITER:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom &vec = *tmp;

                    alloc = true;
                    Atom iter(T_VEC, m_rt->m_gc.allocate_vector(2));
                    if (vec.m_type == T_VEC)
                    {
                        iter.m_d.vec->m_data[0] = Atom(T_INT, -1);
                    }
                    else if (vec.m_type == T_MAP)
                    {
                        AtomMapIterator *mi = new AtomMapIterator(vec);
                        Atom ud(T_UD);
                        ud.m_d.ud = mi;
                        iter.m_d.vec->m_data[0] = ud;
                    }
                    else
                        error("Can't ITER on non map or non vector", vec);

                    iter.m_d.vec->m_data[1] = vec;
                    E_SET(O, iter);
                    break;
                }

                case OP_NEXT:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    E_SET_CHECK_REALLOC(A, A);

                    E_GET(tmp, B);
                    Atom &iter = *tmp;
                    if (iter.m_type != T_VEC && iter.m_d.vec->m_len < 2)
                        error("Bad iterator found in NEXT", iter);

                    Atom *iter_elems = iter.m_d.vec->m_data;

                    if (iter_elems[0].m_type == T_INT)
                    {
                        int64_t &i = iter_elems[0].m_d.i;
                        i++;
                        Atom b(T_BOOL);
                        b.m_d.b = i >= iter_elems[1].m_d.vec->m_len;
                        E_SET(O, b);
                        if (!b.m_d.b)
                        {
                            E_SET(A, iter_elems[1].m_d.vec->at((size_t) i));
                        }
                    }
                    else if (iter_elems[0].m_type == T_UD)
                    {
                        AtomMapIterator *mi =
                            dynamic_cast<AtomMapIterator *>(iter_elems[0].m_d.ud);
                        mi->next();
                        Atom b(T_BOOL);
                        b.m_d.b = !mi->ok();
                        E_SET(O, b);
                        if (!b.m_d.b)
                        {
                            E_SET(A, mi->value());
                        }
                    }
                    break;
                }

                case OP_IKEY:
                {
                    E_SET_CHECK_REALLOC(O, O);
                    E_GET(tmp, A);
                    Atom &iter = *tmp;
                    if (iter.m_type != T_VEC && iter.m_d.vec->m_len < 2)
                        error("Bad iterator found in IKEY", iter);

                    Atom *iter_elems = iter.m_d.vec->m_data;

                    if (iter_elems[0].m_type == T_INT)
                    {
                        E_SET(O, iter_elems[0]);
                    }
                    else if (iter_elems[0].m_type == T_UD)
                    {
                        AtomMapIterator *mi =
                            dynamic_cast<AtomMapIterator *>(iter_elems[0].m_d.ud);
                        E_SET(O, mi->key());
                    }
                    else
                        error("Bad iterator found in IKEY", iter);
                    break;
                }

    #define     DEFINE_NUM_OP_BOOL(opname, oper)              \
                case OP_##opname:                             \
                {                                             \
                    E_SET_CHECK_REALLOC(O, O); \
                    E_GET(tmp, A);                            \
                    Atom &a = *tmp;                           \
                    E_GET(tmp, B);                            \
                    Atom &b = *tmp;                           \
                                                              \
                    Atom o(T_BOOL);                           \
                    if (a.m_type == T_DBL)                    \
                        o.m_d.b = a.to_dbl() oper b.to_dbl(); \
                    else                                      \
                        o.m_d.b = a.to_int() oper b.to_int(); \
                    E_SET(O, o);                              \
                    break;                                    \
                }

    #define     DEFINE_NUM_OP_NUM(opname, oper, neutr)                   \
                case OP_##opname:                                        \
                {                                                        \
                    E_SET_CHECK_REALLOC(O, O);      \
                    E_GET(tmp, A);                                       \
                    Atom &a = *tmp;                                      \
                    E_GET(tmp, B);                                       \
                    Atom &b = *tmp;                                      \
                                                                         \
                    Atom *ot;                                            \
                    E_SET_D_PTR(PE_O, P_O, ot);                          \
                    Atom &o = *ot;                                       \
                    o.m_type = a.m_type;                                 \
                    if (a.m_type == T_DBL)                               \
                        o.m_d.d = a.to_dbl() oper b.to_dbl();            \
                    else if (a.m_type == T_NIL)                          \
                    {                                                    \
                        o.m_type = b.m_type;                             \
                        if (b.m_type == T_DBL)                           \
                        {                                                \
                            o.m_d.d = (double) neutr;                    \
                            o.m_d.d = ((double) neutr) oper b.to_dbl();  \
                        }                                                \
                        else                                             \
                        {                                                \
                            o.m_d.i = (int64_t) neutr;                   \
                            o.m_d.i = ((int64_t) neutr) oper b.to_int(); \
                        }                                                \
                    }                                                    \
                    else                                                 \
                        o.m_d.i = a.to_int() oper b.to_int();            \
                    break;                                               \
                }

                DEFINE_NUM_OP_NUM(ADD, +, 0)
                DEFINE_NUM_OP_NUM(SUB, -, 0)
                DEFINE_NUM_OP_NUM(MUL, *, 1)
                DEFINE_NUM_OP_NUM(DIV, /, 1)

                case OP_MOD:
                {
                    E_SET_CHECK_REALLOC(O, O);

                    E_GET(tmp, A);
                    Atom &a = *tmp;
                    E_GET(tmp, B);
                    Atom &b = *tmp;

                    Atom *ot;
                    E_SET_D_PTR(PE_O, P_O, ot);
                    Atom &o = *ot;
                    o.m_type = a.m_type;
                    if (a.m_type == T_DBL)
                        o.m_d.d = (double) (((int64_t) a.to_dbl()) % ((int64_t) b.to_dbl()));
                    else
                        o.m_d.i = a.to_int() % b.to_int();
                    break;
                }

                DEFINE_NUM_OP_BOOL(GE, >=)
                DEFINE_NUM_OP_BOOL(LE, <=)
                DEFINE_NUM_OP_BOOL(LT, <)
                DEFINE_NUM_OP_BOOL(GT, >)

                case OP_NOP:
                    break;

                default:
                    throw VMException("Unknown VM opcode: " + to_string(m_pc->op));
            }
            m_pc++;

            if (alloc)
            {
                m_rt->m_gc.collect_maybe();
                alloc = false;
            }
        }
    }
    catch (BukaLISPException &)
    {
        throw;
    }
    catch (std::exception &e)
    {
        throw
            add_stack_trace_error(
                BukaLISPException(
                    std::string("Exception in VM: ")
                    + e.what()));
    }

    cont_stack->pop();

//    cout << "STACKS: " << cont_stack->m_len << "; " << frm_stack->m_len << endl;

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span
        = duration_cast<duration<double>>(t2 - t1);
    std::cout << "vm eval run time: "
              << (time_span.count() * 1000)
              << " ms" << std::endl;

    return ret;
#undef P_A
#undef P_B
#undef P_OUT
}
//---------------------------------------------------------------------------


}

/******************************************************************************
* Copyright (C) 2017 Weird Constructor
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
