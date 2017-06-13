case NOP:
    break;

case TRC:
    m_enable_trace_log = op.m_1.i == 1;
    break;

case DUP:
{
    STK_PUSH(STK_AT(1));
    break;
}

case SWAP:
{
    Datum *dt1 = STK_AT(1);
    Datum *dt2 = STK_AT(2);
    STK_POP();
    STK_POP();
    STK_PUSH(dt1);
    STK_PUSH(dt2);
    break;
}

case PUSH_I:
{
    STK_PUSH(new_dt_int(op.m_1.i));
    break;
}

case PUSH_NIL:
{
    STK_PUSH(new_dt_nil());
    break;
}

case PUSH_D:
{
    STK_PUSH(new_dt_dbl(op.m_1.d));
    break;
}

case PUSH_SYM:
{
    STK_PUSH(new_dt_sym(op.m_1.sym));
    break;
}

case PUSH_REF:
{
    STK_AT(1) = new_dt_ref(STK_AT(1));
    break;
}

case SET_REF:
{
    STK_AT(1)->convert_to_ref(STK_AT(2));
    STK_AT(2) = STK_AT(1);
    STK_POP();
    break;
}

case GET_REF:
{
    Datum *dt = STK_AT(1);
    if (dt->m_type == T_REF)
    {
        dt = dt->m_d.ref;
        STK_AT(1) = dt ? dt : new_dt_nil();
    }
    else
    {
        STK_AT(1) = new_dt_nil();
    }
    break;
}

case SUB_D:
{
    Datum *new_dt =
        new_dt_dbl(  STK_AT(2)->to_dbl()
                   - STK_AT(1)->to_dbl());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case SUB_I:
{
    Datum *new_dt =
        new_dt_int(  STK_AT(2)->to_int()
                   - STK_AT(1)->to_int());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case ADD_D:
{
    Datum *new_dt =
        new_dt_dbl(  STK_AT(2)->to_dbl()
                   + STK_AT(1)->to_dbl());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case ADD_I:
{
    Datum *new_dt =
        new_dt_int(  STK_AT(2)->to_int()
                   + STK_AT(1)->to_int());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case MUL_D:
{
    Datum *new_dt =
        new_dt_dbl(  STK_AT(2)->to_dbl()
                   * STK_AT(1)->to_dbl());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case MUL_I:
{
    Datum *new_dt =
        new_dt_int(  STK_AT(2)->to_int()
                   * STK_AT(1)->to_int());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case DIV_D:
{
    Datum *new_dt =
        new_dt_dbl(  STK_AT(2)->to_dbl()
                   / STK_AT(1)->to_dbl());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

case DIV_I:
{
    Datum *new_dt =
        new_dt_int(  STK_AT(2)->to_int()
                   / STK_AT(1)->to_int());
    STK_AT(2) = new_dt; STK_POP();
    break;
}

#define BIN_CMP_OP(cmp_type, op) \
    { Datum *new_dt = new_dt_int((int) \
        (STK_AT(1)->to_##cmp_type() op STK_AT(2)->to_##cmp_type())); \
    STK_AT(2) = new_dt; STK_POP(); \
    break; }

case EQ_I: BIN_CMP_OP(int, ==)
case EQ_D: BIN_CMP_OP(dbl, ==)
case LT_D: BIN_CMP_OP(dbl, <)
case LE_D: BIN_CMP_OP(dbl, <=)
case GT_D: BIN_CMP_OP(dbl, >)
case GE_D: BIN_CMP_OP(dbl, >=)
case LT_I: BIN_CMP_OP(int, <)
case LE_I: BIN_CMP_OP(int, <=)
case GT_I: BIN_CMP_OP(int, >)
case GE_I: BIN_CMP_OP(int, >=)

case PUSH_ENV:
{
    // TODO: Sometime optimize this with vector pools,
    //       because functions are called regularily.
    m_env_stack.push_back(
        m_datum_pool.new_vector(T_VEC, (size_t) op.m_1.i));
    break;
}

case FILL_ENV:
{
    size_t fill_cnt = (size_t) op.m_1.i;

    if (m_env_stack.empty())
        throw VMException("Access to empty env stack", op);
    SimpleVec &sv =
        m_env_stack[m_env_stack.size() - 1]->m_d.vec;

    if (fill_cnt >= sv.len)
        throw VMException(
            "Pushed env too small for fill ("
            + std::to_string(fill_cnt)
            + " >= "
            + std::to_string(sv.len),
            op);

    for (size_t i = 0; i < fill_cnt; i++)
        sv.elems[i] = STK_AT(i + 1);

    for (size_t i = 0; i < fill_cnt; i++)
        STK_POP();
    break;
}

case POP_ENV:
{
    if (m_env_stack.empty())
        throw VMException("Access to empty env stack", op);
    m_env_stack.pop_back();
    break;
}

case POP:
{
    for (int i = 0; i < op.m_1.i; i++)
    {
        STK_POP();
    }
    break;
}

case PUSH_LIST:
{
    Datum *tail = new_dt_ref(STK_AT(1));
    for (int i = 0; i < op.m_1.i; i++)
    {
        Datum *dt = new_dt_ref(STK_AT(i + 2));
        dt->m_next = tail;
        tail = dt;
    }
    for (int i = 0; i < op.m_1.i + 1; i++)
        STK_POP();

    STK_PUSH(tail);
    break;
}

case AT_VEC:
{
    Datum *dt = STK_AT(1);
    if (dt->m_type != T_VEC)
        throw VMException(
            "Can't call this on non vector!", op);
    Datum *dtidx = STK_AT(2);
    size_t idx = (size_t) dtidx->to_int();
    SimpleVec &sv = dt->m_d.vec;
    if (idx > sv.len)
        STK_AT(2) = new_dt_nil();
    else
    {
        Datum *dtget = sv.elems[idx];
        STK_AT(2) = dtget ? dtget : new_dt_nil();
    }
    STK_POP();
    break;
}

case PUSH_VEC:
{
    Datum *len = STK_AT(1);
    size_t veclen = (size_t) len->to_int();

    if (op.m_1.i > (int64_t) veclen)
        throw VMException(
            "Can't create vector with more arguments than size", op);

    Datum *vec = m_datum_pool.new_vector(T_VEC, veclen);
    SimpleVec &sv = vec->m_d.vec;
    for (int i = 0; i < op.m_1.i; i++)
        sv.elems[i] = STK_AT(i + 2);

    for (int i = 0; i < op.m_1.i + 1; i++)
        STK_POP();

    STK_PUSH(vec);
    break;
}

case VEC_LEN:
{
    Datum *dt = STK_AT(1);
    if (dt->m_type != T_VEC)
        throw VMException(
            "Can't call this on non vector!", op);
    SimpleVec &sv = dt->m_d.vec;
    STK_AT(1) = new_dt_int(sv.len);
    break;
}

case IS_VEC:
{
    Datum *dt = STK_AT(1);
    STK_AT(1) = new_dt_int(dt->m_type == T_VEC ? 1 : 0);
    break;
}

case TAIL:
{
    size_t cnt = (size_t) op.m_1.i;
    bool   pop_2 = false;
    Datum *list_item = nullptr;
    if (cnt == 0)
    {
        cnt = (size_t) STK_AT(2)->to_int();
        list_item = STK_AT(1);
        pop_2 = true;
    }
    else
    {
        list_item = STK_AT(1);
    }

    if (cnt == 1)
    {
        list_item = list_item->m_next;
        if (!list_item) list_item = new_dt_nil();
    }
    else
    {
        for (size_t i = 0; i < cnt; i++)
        {
            if (list_item->m_next)
                list_item = list_item->m_next;
            else
            {
                list_item = new_dt_nil();
                break;
            }
        }
    }

    if (pop_2)
    {
        STK_AT(2) = list_item;
        STK_POP();
    }
    else
        STK_AT(1) = list_item;

    break;
}

case LIST_LEN:
{
    Datum *dt = STK_AT(1);
    if (!dt->m_next)
    {
        STK_PUSH(new_dt_nil());
        break;
    }

    int64_t len = 1;
    while (dt)
    {
        dt = dt->m_next;
        len++;
    }

    STK_PUSH(new_dt_int(len));
    break;
}

case IS_LIST:
{
    Datum *dt = STK_AT(1);
    STK_AT(1) = new_dt_int(dt->m_next ? 1 : 0);
    break;
}

case IS_NIL:
{
    Datum *dt = STK_AT(1);
    STK_AT(1) = new_dt_int(dt->m_type == T_NIL ? 1 : 0);
    break;
}

case SET_ENV:
case GET_ENV:
{
    if (m_env_stack.empty())
        throw VMException("Access to empty env stack", op);

    size_t stkoffs = (size_t) op.m_1.i;
    size_t idx     = (size_t) op.m_2.i;

    if (stkoffs >= m_env_stack.size())
        throw VMException("Access to non existing env frame", op);

    SimpleVec &sv =
        m_env_stack[m_env_stack.size() - (stkoffs + 1)]->m_d.vec;
    if (idx >= sv.len)
        throw VMException("Access outside of env frame", op);

    if (op.m_op == SET_ENV)
    {
        sv.elems[idx] = STK_AT(1);
        STK_POP();
    }
    else
    {
        Datum *dt = sv.elems[idx];
        if (!dt) dt = new_dt_nil();
        STK_PUSH(dt);
    }
    break;
}

case JMP:
{
    size_t pos = (size_t) op.m_1.i;

    if (pos < 0 || pos > m_ops.size())
        throw VMException("Invalid JMP address: "
                          + std::to_string(pos), op);

    m_ip = pos - 1;
    break;
}

case BR:
{
    int offs = (int) op.m_1.i;
    int dest = m_ip + offs;

    if (dest < 0 || dest >= (int) m_ops.size())
        throw VMException("Invalid BR offset: "
                          + std::to_string(offs), op);

    m_ip = m_ip + (offs - 1); // -1 because of m_ip++ at the end
    break;
}

case BR_NIF:
{
    int offs = (int) op.m_1.i;
    int dest = m_ip + offs;

    if (dest < 0 || dest >= (int) m_ops.size())
        throw VMException("Invalid BR offset: "
                          + std::to_string(offs), op);

    Datum *dt = STK_AT(1);
    STK_POP();
    if (dt->to_int() == 0)
        m_ip = m_ip + (offs - 1); // -1 because of m_ip++ at the end
    break;
}

case BR_IF:
{
    int offs = (int) op.m_1.i;
    int dest = m_ip + offs;

    if (dest < 0 || dest >= (int) m_ops.size())
        throw VMException("Invalid BR offset: "
                          + std::to_string(offs), op);

    Datum *dt = STK_AT(1);
    STK_POP();
    if (dt->to_int() != 0)
        m_ip = m_ip + (offs - 1); // -1 because of m_ip++ at the end
    break;
}

case PUSH_CLOS_CONT:
{
    size_t skip_offs = (size_t) op.m_1.i;
    STK_PUSH(new_dt_clos(m_ip));
    m_ip += skip_offs;
    break;
}

case PUSH_PRIM:
{
    size_t id = (size_t) op.m_1.i;
    if (m_primitives.size() <= id)
        throw VMException("Unknown primitive id: "
                          + std::to_string(id), op);

    if (!m_primitives[id])
        throw VMException("Invalid primitive at id: "
                          + std::to_string(id), op);

    STK_PUSH(new_dt_prim(m_primitives[id]));
    break;
};

case GET:
case SET:
case TAILCALL:
case CALL:
{
    size_t argc   = (size_t) op.m_1.i;

    auto &dt = STK_AT(1);
    if (   dt->m_type != T_PRIM
        && dt->m_type != T_CLOS
        && dt->m_type != T_EXT)
        throw VMException(
            "Uncallable value on top of the stack: "
            + dt->to_string(), op);

    if (dt->m_type == T_PRIM)
    {
        Datum *ret = (*dt->m_d.func)(this, m_stack, argc);

        for (size_t i = 0; i < argc + 1; i++)
            STK_POP();

        if (!ret) ret = new_dt_int(0);
        STK_PUSH(ret);
    }
    else if (dt->m_type == T_CLOS)
    {
        bool tail = op.m_op == TAILCALL;

        // Make closure one time continuation for returning:
        Datum *dt_cont = tail ? nullptr : new_dt_clos(m_ip);

        SimpleVec &sv = dt->m_d.vec;

        // Write new environment stack:
        size_t new_stack_len = sv.len - 1;
        m_env_stack.resize(new_stack_len + 1);

        for (int i = new_stack_len; i > 0; i--)
            m_env_stack[new_stack_len - i] = sv.elems[i];

        // Create new environment for variables:
        Datum *dt_arg_env =
            m_datum_pool.new_vector(T_VEC, argc);

        // Store all arguments in the new arg env:
        SimpleVec &sv_args = dt_arg_env->m_d.vec;
        for (size_t i = 1; i <= argc; i++)
            sv_args.elems[i - 1] = STK_AT(1 + i);

        m_env_stack[new_stack_len] = dt_arg_env;

        // Store closure one time cont in the stack:
        if (tail)
        {
            for (size_t i = 0; i < argc + 1; i++)
                STK_POP();
        }
        else
        {
            STK_AT(argc + 1) = dt_cont;

            for (size_t i = 0; i < argc; i++)
                STK_POP();
        }

        m_ip = (size_t) sv.elems[0]->to_int();
    }
    else // T_EXT
    {
        Datum *field = argc > 0 ? STK_AT(2) : new_dt_nil();
        Datum *ret = (*dt->m_d.func)(this, m_stack, argc);
        Datum *args = nullptr;

        if (argc > 1 && op.m_op != GET)
        {
            args = m_datum_pool.new_vector(T_VEC, argc - 1);

            SimpleVec &sv_args = dt->m_d.vec;
            for (size_t i = 0; i < (argc - 1); i++)
                sv_args.elems[i] = STK_AT(3 + i);
        }
        else
        {
            args = new_dt_nil();
        }

        Datum *dt_ret = nullptr;
        switch (op.m_op)
        {
            case GET: dt_ret = dt->m_d.ext->get(this, field); break;
            case SET: dt_ret = new_dt_nil();
                               dt->m_d.ext->set(this, field, args); break;
            default:  dt_ret = dt->m_d.ext->call(this, field, args); break;
        };
        if (!dt_ret) dt_ret = new_dt_nil();

        for (size_t i = 0; i < argc + 1; i++)
            STK_POP();

        STK_PUSH(dt_ret);
    }

    break;
}

case RETURN:
{
    Datum *dt_ret  = STK_AT(1);
    Datum *dt_cont = STK_AT(2);

    if (dt_cont->m_type != T_CLOS)
    {
        throw VMException(
            "Continuation Closure not found on Stack!", op);
    }

    SimpleVec &sv = dt_cont->m_d.vec;

    // Write new environment stack:
    size_t new_stack_len = sv.len - 1;
    m_env_stack.resize(new_stack_len);

    for (int i = new_stack_len; i > 0; i--)
        m_env_stack[new_stack_len - i] =
            sv.elems[i];

    m_ip = (size_t) sv.elems[0]->to_int();
    STK_AT(2) = STK_AT(1);
    STK_POP();

//                        Get return value STK_AT(1)
//                            Restore m_env_stack from closure on stack STK_AT(2)

//                        Restore m_ip from closure at STK_AT(2)
//                        Replace STK_AT(2) with STK_AT(1) and pop 1 value
    break;
}

case DBG_DUMP_STACK:
{
    dump_stack("DBG_DUMP_STACK", op);
    break;
}

case DBG_DUMP_ENVSTACK:
{
    dump_envstack("DBG_DUMP_ENVSTACK", op);
    break;
}

case PUSH_GC_USE_COUNT:
{
    STK_PUSH(
        new_dt_int(
            m_datum_pool.get_datums_in_use_count()));
    break;
}

default:
    throw VMException("Unknown OP", op);
