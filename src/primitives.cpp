// Copyright (C) 2017 Weird Constructor
// For more license info refer to the the bottom of this file.

#define BIN_OP_LOOPS(op) \
        if (args.m_len > 1) { \
            bool as_dbl = false; \
            for (size_t i = 0; i < args.m_len; i++) \
                if (args.m_data[i].m_type == T_DBL) { as_dbl = true; break ; } \
            if (as_dbl) { \
                out.set_dbl(out.to_dbl()); \
                double &o = out.m_d.d; \
                for (size_t i = 1; i < args.m_len; i++) \
                    o = o op args.m_data[i].to_dbl(); \
            } else { \
                out.set_int(out.to_int()); \
                int64_t &o = out.m_d.i; \
                for (size_t i = 1; i < args.m_len; i++) \
                    o = o op args.m_data[i].to_int(); \
            } \
        }

#define NO_ARG_NIL if (args.m_len <= 0) { out = Atom(); return; }

#define REQ_S_ARG(arg, msg) \
    if (   (arg).m_type != T_STR \
        && (arg).m_type != T_SYM \
        && (arg).m_type != T_KW) \
    { \
        error(msg, (arg)); \
    }

START_PRIM()
    NO_ARG_NIL;
    out = args.m_data[0];
    BIN_OP_LOOPS(+)
END_PRIM(+);

START_PRIM()
    NO_ARG_NIL;
    out = args.m_data[0];
    BIN_OP_LOOPS(*)
END_PRIM(*);

START_PRIM()
    NO_ARG_NIL;
    out = args.m_data[0];
    if (args.m_len == 1)
    {
        if (out.m_type == T_INT) out.set_int(1 / out.to_int());
        else                     out.set_dbl(1.0 / out.to_dbl());
        return;
    }
    BIN_OP_LOOPS(/)
END_PRIM(/);

START_PRIM()
    NO_ARG_NIL;
    out = args.m_data[0];
    if (args.m_len == 1)
    {
        if      (out.m_type == T_DBL) out.m_d.d *= -1.0;
        else if (out.m_type == T_INT) out.m_d.i *= -1;
        else                          out.set_int(- out.to_int());
        return;
    }
    BIN_OP_LOOPS(-)
END_PRIM(-);

#define REQ_GT_ARGC(prim, cnt)    if (args.m_len < cnt)  error("Not enough arguments to " #prim ", expected " #cnt);
#define REQ_EQ_ARGC(prim, cnt)    if (args.m_len != cnt) error("Wrong number of arguments to " #prim ", expected " #cnt);
#define BIN_CMP_OP_NUM(op) \
    out = Atom(T_BOOL); \
    bool as_dbl = false; \
    for (size_t i = 0; i < args.m_len; i++) \
        if (args.m_data[i].m_type == T_DBL) { as_dbl = true; break ; } \
    Atom last = args.m_data[0]; \
    for (size_t i = 1; i < args.m_len; i++) \
    { \
        if (as_dbl) \
            out.m_d.b = last.to_dbl() op args.m_data[i].to_dbl(); \
        else \
            out.m_d.b = last.to_int() op args.m_data[i].to_int(); \
        if (!out.m_d.b) return; \
        last = args.m_data[i]; \
    }

START_PRIM()
    REQ_GT_ARGC(=, 2);
    BIN_CMP_OP_NUM(==);
END_PRIM(=);
START_PRIM()
    REQ_GT_ARGC(<, 2);
    BIN_CMP_OP_NUM(<);
END_PRIM(<);
START_PRIM()
    REQ_GT_ARGC(>, 2);
    BIN_CMP_OP_NUM(>);
END_PRIM(>);
START_PRIM()
    REQ_GT_ARGC(<=, 2);
    BIN_CMP_OP_NUM(<=);
END_PRIM(<=);
START_PRIM()
    REQ_GT_ARGC(>=, 2);
    BIN_CMP_OP_NUM(>=);
END_PRIM(>=);

#define A0  (args.m_data[0])
#define A1  (args.m_data[1])
#define A2  (args.m_data[2])
#define A3  (args.m_data[3])

START_PRIM()
    REQ_EQ_ARGC(abs, 1);
    if (A0.m_type == T_DBL)
        out.set_dbl(A0.m_d.d < 0 ? -A0.m_d.d : A0.m_d.d);
    else
        out.set_int(A0.to_int() < 0 ? -A0.to_int() : A0.to_int());
END_PRIM(abs);

START_PRIM()
    REQ_GT_ARGC(number->string, 0);
    if (args.m_len > 2)
        error("'number->string' too many arguments");
    if (args.m_len == 1)
    {
        out = Atom(T_STR, m_rt->m_gc.new_symbol(A0.to_write_str()));
    }
    else
    {
        if (A0.m_type == T_DBL)
            out = Atom(T_STR, m_rt->m_gc.new_symbol(A0.to_write_str()));
        else
        {
            int base  = (int) A1.to_int();
            int64_t i = A0.to_int();

            if (base < 2 || base > 36)
                error("'number->string' supports only base >= 2 and base <= 36",
                      A1);

            // from http://en.cppreference.com/w/cpp/numeric/math/div
            std::string buf;
            std::lldiv_t dv{};
            dv.quot = i;
            do {
                dv = std::lldiv(dv.quot, base);
                buf += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[std::abs(dv.rem)];
            } while(dv.quot);
            if (i < 0) buf += "-";
            std::string out_buf(buf.rbegin(), buf.rend());
            out = Atom(T_STR, m_rt->m_gc.new_symbol(out_buf));
        }
    }
END_PRIM(number->string)

START_PRIM()
    REQ_EQ_ARGC(any->inexact, 1);
    out.set_dbl(A0.to_dbl());
END_PRIM(any->inexact)

START_PRIM()
    REQ_GT_ARGC(string->number, 0);
    if (args.m_len > 2)
        error("'string->number' too many arguments");
    int base = 0;
    if (args.m_len == 2)
    {
        base = (int) A1.to_int();
        if (base < 2 || base > 36)
            error("'string->number' support only base >= 2"
                  "and base <= 36",
                  A1);
    }
    out = A0.any_to_number(base);
END_PRIM(string->number);

START_PRIM()
    REQ_EQ_ARGC(any->number, 1);
    out = A0.any_to_number(0);
END_PRIM(any->number);

START_PRIM()
    REQ_EQ_ARGC(any->exact, 1);
    out.set_int(A0.any_to_exact());
END_PRIM(any->exact);

START_PRIM()
    REQ_EQ_ARGC(any->inexact, 1);
    out.set_dbl(A0.any_to_inexact());
END_PRIM(any->inexact);

#define DEF_SIMPLE_NUM_PRIM(name, std_func) \
START_PRIM() \
    REQ_EQ_ARGC(name, 1); \
    if (A0.m_type == T_DBL) \
        out.set_dbl(std_func(A0.m_d.d)); \
    else \
        out.set_int(A0.any_to_exact()); \
END_PRIM(name)

DEF_SIMPLE_NUM_PRIM(floor, std::floor)
DEF_SIMPLE_NUM_PRIM(truncate, std::trunc)
DEF_SIMPLE_NUM_PRIM(ceiling, std::ceil)
DEF_SIMPLE_NUM_PRIM(round, (double) std::llround)

#define DEF_SIMPLE_MATHFUNC_PRIM(name, std_func) \
START_PRIM() \
    REQ_EQ_ARGC(name, 1); \
    out.set_dbl(std_func((double) A0.to_dbl())); \
END_PRIM(name)

DEF_SIMPLE_MATHFUNC_PRIM(exp, std::exp)
DEF_SIMPLE_MATHFUNC_PRIM(sin, std::sin)
DEF_SIMPLE_MATHFUNC_PRIM(cos, std::cos)
DEF_SIMPLE_MATHFUNC_PRIM(tan, std::tan)
DEF_SIMPLE_MATHFUNC_PRIM(asin, std::asin)
DEF_SIMPLE_MATHFUNC_PRIM(acos, std::acos)

START_PRIM()
    REQ_GT_ARGC(atan, 0);
    if (args.m_len > 2)
        error("'atan' too many arguments");
    if (args.m_len == 1)
        out.set_dbl(std::atan(A0.to_dbl()));
    else
        out.set_dbl(std::atan2(A0.to_dbl(), A1.to_dbl()));
END_PRIM(atan)

START_PRIM()
    REQ_GT_ARGC(log, 1);
    if (args.m_len > 2)
        error("'log' too many arguments");
    if (args.m_len == 1)
        out.set_dbl(std::log(A0.to_dbl()));
    else
        out.set_dbl(std::log(A0.to_dbl()) / std::log(A1.to_dbl()));
END_PRIM(log)

START_PRIM()
    REQ_EQ_ARGC(string-downcase, 1)
    std::string in = A0.to_display_str();
    std::transform(in.begin(), in.end(), in.begin(), ::tolower);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(in));
END_PRIM_DOC(string-downcase,
"@strings procedure (string-downcase _value_)\n"
"\n"
"Returns the lower case string version of _value_. Currently only supports ASCII properly.")

START_PRIM()
    REQ_EQ_ARGC(string-upcase, 1)
    std::string in = A0.to_display_str();
    std::transform(in.begin(), in.end(), in.begin(), ::toupper);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(in));
END_PRIM_DOC(string-upcase,
"@strings procedure (string-upcase _value_)\n"
"\n"
"Returns the upper case string version of _value_. Currently only supports ASCII properly.")

START_PRIM()
	REQ_GT_ARGC(substring, 2);
	std::string s = A0.to_display_str();
	size_t start = (size_t) A1.to_int();
	size_t len =
		args.m_len > 2
		? (((size_t) A2.to_int())
		   - start)
		: std::string::npos;
	if (start < s.size())
		s = s.substr(start, len);
	out = Atom(T_STR, m_rt->m_gc.new_symbol(s));
END_PRIM_DOC(substring,
"@strings procedure (substring _string_ _start_)\n"
"@strings procedure (substring _string_ _start_ _end_)\n"
"\n"
"Returns the substring of _string_ between _start_ and _end_.\n"
"The indexes _start_ and _end_ are 0 based, and _end_ is exclusive.\n"
"If _end_ is omitted, the rest of the string starting at _start_\n"
"is returned.\n"
"\n"
"    (substring \"abcdef\" 0 1) ;=> \"a\"\n"
"    (substring \"abcdef\" 1)   ;=> \"bcdef\"\n"
"    (substring \"abcdef\" 2 4) ;=> \"cd\"\n")

START_PRIM()
    out = Atom(T_VEC);
    AtomVec *l = m_rt->m_gc.allocate_vector(args.m_len);
    out.m_d.vec = l;
    for (size_t i = 0; i < args.m_len; i++)
        l->m_data[i] = args.m_data[i];
    l->m_len = args.m_len;
END_PRIM(list);

START_PRIM()
    REQ_EQ_ARGC(eq?, 2);
    out.set_bool(A0.eqv(A1));
END_PRIM_DOC(eqv?,
"@predicates procedure (eqv? _a-value_ _b-value_)\n"
"\n"
"Returns #true if the type of _a-value_ and _b-value_ are equal\n"
"and their values are equal. This procedure does only check for\n"
"referencial equality for complicated types like lists or maps.\n"
"\n"
"    (eqv? 'a 'a)                 ;=> #t\n"
"    (eqv? 'a 'b)                 ;=> #f\n"
"    (eqv? 2 2)                   ;=> #t\n"
"    (eqv? 2 2.0)                 ;=> #f\n"
"    (eqv? nil nil)               ;=> #t\n"
"    (eqv? 100000000 100000000)   ;=> #t\n"
"    (eqv? (cons 1 2) (cons 1 2)) ;=> #f\n"
"    (eqv? (lambda () 1)\n"
"          (lambda () 2))         ;=> #f\n"
"    (let ((p (lambda (x) x)))\n"
"      (eqv? p p))                ;=> #t\n"
"    (eqv? #f 'nil)               ;=> #f\n"
"    (eqv? a: a:)                 ;=> #t\n"
"    (eqv? a: 'a)                 ;=> #f\n"
"    (eqv? [1 2 3] [1 2 3])       ;=> #f\n"
"    (let ((a [1 2 3]))\n"
"      (eqv? a a))                ;=> #t\n"
"    (eqv? {a: 1 b: 2}\n"
"          {a: 1 b: 2})           ;=> #f\n"
"    (let ((a {a: 1 b: 2}))\n"
"      (eqv? a a))                ;=> #t\n"
);

START_PRIM()
    REQ_EQ_ARGC(equal?, 2);
    out.set_bool(A0.equal(A1));
END_PRIM_DOC(equal?,
"@predicates procedure (equal? _a-value_ _b-value_)\n"
"\n"
"Returns #true if _a-value_ is (structurally) equal to _b-value_.\n"
"`equal?` compares _a-value_ and _b-value_ recursively on equality,\n"
"for values other than lists and maps it is equal to the result of `eqv?`.\n"
"\n"
"In case of cyclic data structures this routine will not work properly.\n"
"\n"
"    (equal? 'a 'a)               ;=> #t\n"
"    (equal? '(a) '(a))           ;=> #t\n"
"    (equal? '(a (b) c)\n"
"            '(a (b) c))          ;=> #t\n"
"    (equal? '(a (c) c)\n"
"            '(a (b) c))          ;=> #f\n"
"    (equal? '(a c c)\n"
"            '(a (b) c))          ;=> #f\n"
"    (equal? \"abc\" \"abc\")     ;=> #t\n"
"    (equal? 2 2)                 ;=> #t\n"
"    (equal? {a: [1 2 3] b: 3}\n"
"            {b: 3 a: [1 2 3]})   ;=> #t\n"
"    (equal? {b: [1 2 3]}\n"
"            {b: 3 a: [1 2 3]})   ;=> #f\n"
);

START_PRIM()
    REQ_EQ_ARGC(not, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.is_false();
END_PRIM(not);

START_PRIM()
    REQ_EQ_ARGC(symbol?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_SYM;
END_PRIM(symbol?);

START_PRIM()
    REQ_EQ_ARGC(keyword?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_KW;
END_PRIM(keyword?);

START_PRIM()
    REQ_EQ_ARGC(nil?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_NIL;
END_PRIM_DOC(nil?,
"@lists procedure (nil? _value_)\n"
"\n"
"Returns true if _value_ is the `nil` value.\n"
"And only then.\n"
"\n"
"    (nil?   nil)     ;=> #t\n"
"    (nil?   [])      ;=> #f\n"
"    (nil?   '())     ;=> #f\n"
"    (nil?   {})      ;=> #f\n"
"    (nil?   0)       ;=> #f\n"
"    (nil?   "")      ;=> #f\n"
);

START_PRIM()
    REQ_EQ_ARGC(null?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_VEC && A0.m_d.vec->m_len == 0;
END_PRIM_DOC(null?,
"@lists (null? _list_)\n"
"\n"
"Returns `#t` if _list_ is an empty list.\n"
"Same as `(empty? ...)`.\n"
"\n"
"But don't mix this up with `(nil? ...)`, which detects\n"
"the `nil` value.\n"
"\n"
"    (null?  [1 2 3]) ;=> #f\n"
"    (null?  '())     ;=> #t\n"
"    (null?  [])      ;=> #t\n"
"    (null?  "")      ;=> #t\n"
"\n"
"See also: `nil?`\n"
);

START_PRIM()
    REQ_EQ_ARGC(exact?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_INT;
END_PRIM(exact?);

START_PRIM()
    REQ_EQ_ARGC(inexact?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_DBL;
END_PRIM(inexact?);

START_PRIM()
    REQ_EQ_ARGC(number?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_DBL || A0.m_type == T_INT;
END_PRIM(number?);

START_PRIM()
    REQ_EQ_ARGC(string?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_STR;
END_PRIM(string?);

START_PRIM()
    REQ_EQ_ARGC(boolean?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_BOOL;
END_PRIM(boolean?);

START_PRIM()
    REQ_EQ_ARGC(list?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_VEC;
END_PRIM(list?);

START_PRIM()
    REQ_EQ_ARGC(map?, 1);
    out = Atom(T_BOOL);
    out.m_d.b = A0.m_type == T_MAP;
END_PRIM(map?);

START_PRIM()
    REQ_EQ_ARGC(procedure?, 1);
    out = Atom(T_BOOL);
    out.m_d.b =
           A0.m_type == T_PRIM
        || A0.m_type == T_CLOS
        || (A0.m_type == T_UD && A0.m_d.ud->type() == "BKL-VM-PROG");
END_PRIM(procedure?);

START_PRIM()
    REQ_EQ_ARGC(coroutine?, 1);
    out.set_bool(
        A0.m_type == T_CLOS
        && !A0.m_d.vec->at(VM_CLOS_IS_CORO).is_false());
END_PRIM(coroutine?);

START_PRIM()
    REQ_EQ_ARGC(coroutine?, 1);
    out.set_bool(
        A0.m_type == T_CLOS
        && !A0.m_d.vec->at(VM_CLOS_IS_CORO).is_false()
        && A0.m_d.vec->at(VM_CLOS_IS_CORO).m_type == T_VEC);
END_PRIM(coroutine-yielded?);

START_PRIM()
    REQ_EQ_ARGC(@, 2);
    if (A1.m_type == T_VEC)
    {
        int64_t idx = A0.to_int();
        if (idx < 0) out = Atom();
        else         out = A1.at((size_t) idx);
    }
    else if (A1.m_type == T_MAP)
    {
        out = A1.at(A0);
    }
    else
        error("Can apply '@' only to lists or maps", A1);
END_PRIM(@);

START_PRIM()
    REQ_EQ_ARGC(@!, 3);
    if (A1.m_type == T_VEC)
    {
        int64_t i = A0.to_int();
        if (i >= 0)
            A1.m_d.vec->set((size_t) A0.to_int(), A2);
        out = A2;
    }
    else if (A1.m_type == T_MAP)
    {
        A1.m_d.map->set(A0, A2);
        out = A2;
    }
    else
        error("Can apply '@!' only to lists or maps", A1);
END_PRIM(@!);

START_PRIM()
    REQ_EQ_ARGC(type, 1);
    out = Atom(T_SYM);
    switch (A0.m_type)
    {
        // TODO: use X macro!
        case T_NIL:    out.m_d.sym = m_rt->m_gc.new_symbol("nil");       break;
        case T_INT:    out.m_d.sym = m_rt->m_gc.new_symbol("exact");     break;
        case T_DBL:    out.m_d.sym = m_rt->m_gc.new_symbol("inexact");   break;
        case T_BOOL:   out.m_d.sym = m_rt->m_gc.new_symbol("boolean");   break;
        case T_STR:    out.m_d.sym = m_rt->m_gc.new_symbol("string");    break;
        case T_SYM:    out.m_d.sym = m_rt->m_gc.new_symbol("symbol");    break;
        case T_KW:     out.m_d.sym = m_rt->m_gc.new_symbol("keyword");   break;
        case T_VEC:    out.m_d.sym = m_rt->m_gc.new_symbol("list");      break;
        case T_MAP:    out.m_d.sym = m_rt->m_gc.new_symbol("map");       break;
        case T_PRIM:   out.m_d.sym = m_rt->m_gc.new_symbol("procedure"); break;
        case T_SYNTAX: out.m_d.sym = m_rt->m_gc.new_symbol("syntax");    break;
        case T_CLOS:   out.m_d.sym = m_rt->m_gc.new_symbol("procedure"); break;
        case T_UD:     out.m_d.sym = m_rt->m_gc.new_symbol("userdata");  break;
        case T_C_PTR:  out.m_d.sym = m_rt->m_gc.new_symbol("c-pointer"); break;
        default:       out.m_d.sym = m_rt->m_gc.new_symbol("<unknown>"); break;
    }
END_PRIM(type)

START_PRIM()
    REQ_EQ_ARGC(length, 1);
    out = Atom(T_INT);
    if (A0.m_type == T_VEC)         out.m_d.i = A0.m_d.vec->m_len;
    else if (A0.m_type == T_MAP)    out.m_d.i = A0.m_d.map->size();
	else if (   A0.m_type == T_STR
	         || A0.m_type == T_SYM
	         || A0.m_type == T_KW)
		out.m_d.i = A0.m_d.sym->m_str.size();
    else
		error("'length' can only be used on a map, list, string, symbol and keyword");
END_PRIM_DOC(length,
"@basics procedure (length _string/symbol/keyword_)\n"
"@basics procedure (length _list_)\n"
"@basics procedure (length _map_)\n"
"\n"
"This procedure returns the length of the passed value.\n"
"It must not be confused with the `size` procedure.\n"
"If used on a string value (or symbol, or keyword) it returns\n"
"the numbers of bytes used to represent it.\n"
"If used on a list, it returns the number of elements of that list.\n"
"If used on a map, it returns the number of stored values\n"
"(or key/value pairs) in that map.\n"
"\n"
"    (length \"abcdef\")    ;=> 6\n"
"    (length abc:)          ;=> 3\n"
"    (length 'abc)          ;=> 3\n"
"    (length [1 2 3])       ;=> 3\n"
"    (length {a: 10 b: 20}) ;=> 2\n")

START_PRIM()
    REQ_EQ_ARGC(push!, 2);

    if (A0.m_type != T_VEC)
        error("Can't push onto something that is not a list", A0);

    A0.m_d.vec->push(A1);
    out = A1;
END_PRIM(push!)

START_PRIM()
    REQ_EQ_ARGC(pop!, 1);

    if (A0.m_type != T_VEC)
        error("Can't pop from something that is not a list", A0);

    Atom *a = A0.m_d.vec->last();
    if (a) out = *a;
    A0.m_d.vec->pop();
END_PRIM(pop!)

START_PRIM()
    REQ_EQ_ARGC(unshift!, 2)

    if (A0.m_type != T_VEC)
        error("Can't unshift onto something that is not a list", A0);

    A0.m_d.vec->unshift(A1);
    out = A1;
END_PRIM(unshift!)

START_PRIM()
    REQ_EQ_ARGC(shift!, 1)

    if (A0.m_type != T_VEC)
        error("Can't shift from something that is not a list", A0);

    out = *(A0.m_d.vec->first());
    A0.m_d.vec->shift();
END_PRIM(shift!)

START_PRIM()
    REQ_EQ_ARGC(set-length!, 2);

    if (A0.m_type != T_VEC)
        error("Can only set-length! on a list!", A0);

    size_t new_len = (size_t) A1.to_int();
    AtomVec *v = A0.m_d.vec;
    if (v->m_len < new_len)
        v->check_size(new_len - 1);
    else
        v->m_len = new_len;
    out = A0;
END_PRIM(set-length!)

START_PRIM()
    REQ_EQ_ARGC(bkl-make-vm-prog, 1);
    out = PROG::create_prog_from_info(m_rt->m_gc, A0);
END_PRIM(bkl-make-vm-prog)

START_PRIM()
    REQ_EQ_ARGC(bkl-prog-serialize, 2);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(atom2cpp(A0.to_display_str(), A1)));
END_PRIM(bkl-prog-serialize)

START_PRIM()
    REQ_GT_ARGC(bkl-run-vm, 1);

    AtomVec *prog_args = nullptr;
    if (args.m_len > 1 && A1.m_type != T_NIL)
    {
        if (A1.m_type != T_VEC)
            error("Can execute VM only with a list as argument list", A1);
        prog_args = A1.m_d.vec;
    }

    if (!m_vm)
        error("Can't execute VM, no VM instance loaded into interpreter", A0);

    out = m_vm->eval(A0, prog_args);
END_PRIM(bkl-run-vm)

START_PRIM()
    REQ_EQ_ARGC(bkl-get-vm-modules, 0);

    if (!m_vm)
        error("Can't get VM modules, no VM instance loaded into interpreter", A0);

    if (!m_modules)
        error("Can't get VM modules, no modules defined", A0);

    out = Atom(T_MAP, m_modules);
END_PRIM(bkl-get-vm-modules);

START_PRIM()
    REQ_GT_ARGC(error, 1);

    Atom a(T_VEC);
    a.m_d.vec = m_rt->m_gc.allocate_vector(args.m_len - 1);
    for (size_t i = 1; i < args.m_len; i++)
        a.m_d.vec->m_data[i - 1] = args.m_data[i];
    a.m_d.vec->m_len = args.m_len - 1;
    error(A0.to_display_str(), a);
END_PRIM(error)

START_PRIM()
    REQ_EQ_ARGC(raise, 1);
    throw VMRaise(m_rt->m_gc, A0);
END_PRIM(raise)

START_PRIM()
    REQ_EQ_ARGC(bkl-error, 2);
    throw
        add_stack_trace_error(
            BukaLISPException(A0.to_display_str())
        .push(A1));
END_PRIM(bkl-error)

START_PRIM()
    REQ_EQ_ARGC(bkl-set-meta, 3);
    m_rt->m_gc.set_meta_register(A0, (size_t) A1.to_int(), A2);
    out = A2;
END_PRIM(bkl-set-meta)

START_PRIM()
    REQ_EQ_ARGC(bkl-get-meta, 1);
    out = A0.meta();
END_PRIM(bkl-get-meta)

START_PRIM()
    REQ_GT_ARGC(display, 1);

    for (size_t i = 0; i < args.m_len; i++)
    {
        std::cout << args.m_data[i].to_display_str();
        if (i < (args.m_len - 1))
            std::cout << " ";
    }

    out = args.m_data[args.m_len - 1];
END_PRIM(display)

START_PRIM()
    REQ_EQ_ARGC(newline, 0);
    std::cout << std::endl;
END_PRIM(newline)

START_PRIM()
    REQ_GT_ARGC(displayln, 1);

    for (size_t i = 0; i < args.m_len; i++)
    {
        std::cout << args.m_data[i].to_display_str();
        if (i < (args.m_len - 1))
            std::cout << " ";
    }
    std::cout << std::endl;

    out = args.m_data[args.m_len - 1];
END_PRIM(displayln)

START_PRIM()
    REQ_EQ_ARGC(atom-id, 1);
    out = Atom(T_INT, A0.id());
END_PRIM(atom-id)

START_PRIM()
    REQ_GT_ARGC(str, 0);
    std::string out_str;
    for (size_t i = 0; i < args.m_len; i++)
        out_str += args.m_data[i].to_display_str();
    out = Atom(T_STR, m_rt->m_gc.new_symbol(out_str));
END_PRIM(str)

START_PRIM()
    REQ_GT_ARGC(str-join, 1);
    std::string sep = A0.to_display_str();
    std::string out_str;
    for (size_t i = 1; i < args.m_len; i++)
    {
        if (i > 1)
            out_str += sep;
        out_str += args.m_data[i].to_display_str();
    }
    out = Atom(T_STR, m_rt->m_gc.new_symbol(out_str));
END_PRIM(str-join)

START_PRIM()
    REQ_EQ_ARGC(symbol->string, 1)
    if (A0.m_type != T_SYM)
        error("'symbol->string' expected symbol as argument", A0);
    out = Atom(T_STR, A0.m_d.sym);
END_PRIM(symbol->string)

START_PRIM()
    REQ_EQ_ARGC(keyword->string, 1)
    if (A0.m_type != T_KW)
        error("'keyword->string' expected keyword as argument", A0);
    out = Atom(T_STR, A0.m_d.sym);
END_PRIM(keyword->string)

START_PRIM()
    REQ_EQ_ARGC(string->symbol, 1)
    if (A0.m_type != T_STR)
        error("'string->symbol' expected string as argument", A0);
    out = Atom(T_SYM, A0.m_d.sym);
END_PRIM(string->symbol)

START_PRIM()
    REQ_EQ_ARGC(string->keyword, 1)
    if (A0.m_type != T_STR)
        error("'string->keyword' expected string as argument", A0);
    out = Atom(T_KW, A0.m_d.sym);
END_PRIM(string->keyword)

START_PRIM()
    REQ_EQ_ARGC(last, 1);
    if (A0.m_type != T_VEC)
        error("Can't 'last' on a non-list", A0);
    AtomVec *avl = A0.m_d.vec;
    if (avl->m_len <= 0)
    {
        out = Atom();
        return;
    }
    out = avl->m_data[avl->m_len - 1];
END_PRIM(last)

START_PRIM()
    REQ_EQ_ARGC(first, 1);
    if (A0.m_type != T_VEC)
        error("Can't 'first' on a non-list", A0);
    out = A0.at(0);
END_PRIM(first)

START_PRIM()
    REQ_EQ_ARGC(take, 2);
    if (A0.m_type != T_VEC)
        error("Can't 'take' on a non-list", A0);
    size_t ti  = (size_t) A1.to_int();
    size_t len = (size_t) A0.m_d.vec->m_len;
    if (ti > len) ti = len;
    AtomVec *av = m_rt->m_gc.allocate_vector(ti);
    for (size_t i = 0; i < len && i < ti; i++)
        av->m_data[i] = A0.m_d.vec->m_data[i];
    av->m_len = ti;
    out = Atom(T_VEC, av);
END_PRIM(take)

START_PRIM()
    REQ_EQ_ARGC(drop, 2);
    if (A0.m_type != T_VEC)
        error("Can't 'drop' on a non-list", A0);
    size_t di  = (size_t) A1.to_int();
    size_t len = (size_t) A0.m_d.vec->m_len;
    if (len < di)
    {
        out = Atom(T_VEC, m_rt->m_gc.allocate_vector(0));
        return;
    }
    len -= di;
    AtomVec *av = m_rt->m_gc.allocate_vector(len);
    for (size_t i = 0; i < len; i++)
        av->m_data[i] = A0.m_d.vec->m_data[i + di];
    av->m_len = len;
    out = Atom(T_VEC, av);
END_PRIM(drop)

START_PRIM()
    REQ_GT_ARGC(append, 0);
    AtomVec *av = m_rt->m_gc.allocate_vector(args.m_len);
    for (size_t i = 0; i < args.m_len; i++)
    {
        Atom &a = args.m_data[i];
        if (a.m_type == T_VEC)
        {
            Atom *v = a.m_d.vec->m_data;
            for (size_t j = 0; j < a.m_d.vec->m_len; j++)
                av->push(v[j]);
        }
        else if (a.m_type == T_MAP)
        {
            ATOM_MAP_FOR(i, a.m_d.map)
            {
                av->push(MAP_ITER_KEY(i));
                av->push(MAP_ITER_VAL(i));
            }
        }
        else
        {
            av->push(a);
        }
    }
    out.set_vec(av);
END_PRIM(append)

START_PRIM()
    REQ_EQ_ARGC(reverse, 1);
    if (A0.m_type != T_VEC)
        error("Can't reverse non-vector", A0);
    AtomVec *av = m_rt->m_gc.allocate_vector(A0.m_d.vec->m_len);
    for (size_t i = A0.m_d.vec->m_len; i > 0; i--)
        av->push(A0.m_d.vec->m_data[i - 1]);
    out.set_vec(av);
END_PRIM(reverse)

START_PRIM()
    REQ_EQ_ARGC(write-str, 1);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(A0.to_write_str()));
END_PRIM(write-str);

START_PRIM()
    REQ_EQ_ARGC(pp-str, 1);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(A0.to_write_str(true)));
END_PRIM(pp-str);

START_PRIM()
    REQ_GT_ARGC(read-str, 1);
    if (args.m_len > 1)
        out = m_rt->read(A1.to_display_str(), A0.to_display_str());
    else
        out = m_rt->read("", A0.to_display_str());
END_PRIM(read-str)

START_PRIM()
    REQ_EQ_ARGC(userdata->atom, 1);
    out = expand_userdata_to_atoms(&(m_rt->m_gc), A0);
END_PRIM(userdata->atom)

START_PRIM()
    REQ_EQ_ARGC(atom->userdata, 1);
    AtomMap *refmap = m_rt->m_gc.allocate_map();
    out = PROG::repack_expanded_userdata(m_rt->m_gc, A0, refmap);
END_PRIM(atom->userdata)

START_PRIM()
    REQ_EQ_ARGC(size, 1);
    out = Atom(T_INT, (int64_t) A0.size());
END_PRIM_DOC(size,
"@basics procedure (size _value_)\n"
"\n"
"This procdure returns the size of the data structure passed as _value_.\n"
"If _value_ is a list or map it counts the elements that make them up.\n"
"It recursively walks lists and maps to sum up all elements, any non list or\n"
"non map is counted as one element (a key/value pair in a map counts as 2).\n"
"If _value_ is a string, symbol or keyword it returns the number of bytes\n"
"that are used to represent it.\n"
"For any other type of _value_ some internal platform dependent size\n"
"that has something to do with the internal storage space, is returned.\n")

START_PRIM()
    REQ_EQ_ARGC(sys-slurp-file, 1);
    REQ_S_ARG(A0,
        "'bkl-slurp-file' requires a string, symbol "
        "or keyword as first argument.");
    out = Atom(T_STR, m_rt->m_gc.new_symbol(slurp_str(A0.m_d.sym->m_str)));
END_PRIM(sys-slurp-file)

START_PRIM()
    REQ_EQ_ARGC(sys-write-file, 2);
    REQ_S_ARG(A0,
        "'bkl-write-file' requires a string, symbol "
        "or keyword as first argument.");
    out = Atom(T_BOOL, write_str(A0.m_d.sym->m_str, A1.to_display_str()));
END_PRIM(sys-write-file)

START_PRIM()
    REQ_EQ_ARGC(sys-path-separator, 0);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(BKL_PATH_SEP));
END_PRIM(sys-path-separator)

START_PRIM()
    REQ_EQ_ARGC(sys-path-split, 1);
    std::string s = A0.to_display_str();
    size_t pos = s.find_last_of("/\\");
    AtomVec *av = m_rt->m_gc.allocate_vector(2);
    if (pos == string::npos)
    {
        av->push(Atom(T_STR, m_rt->m_gc.new_symbol(s)));
        av->push(Atom(T_STR, m_rt->m_gc.new_symbol("")));
    }
    else
    {
        av->push(Atom(T_STR, m_rt->m_gc.new_symbol(s.substr(0, pos))));
        av->push(Atom(T_STR, m_rt->m_gc.new_symbol(s.substr(pos + 1))));
    }
    out = Atom(T_VEC, av);
END_PRIM(sys-path-split)

START_PRIM()
    REQ_EQ_ARGC(sys-platform, 0);
    out = Atom(T_STR, m_rt->m_gc.new_symbol(BKL_PLATFORM));
END_PRIM(sys-platform)

START_PRIM()
    REQ_GT_ARGC(apply, 1);

    if (!m_vm)
        error("Can't run 'apply' without VM.", A0);

//    cout << "APPLY PRIM: " << Atom(T_VEC, &args).to_write_str() << endl;

    if (args.m_len == 1)
    {
        AtomVec *av = m_rt->m_gc.allocate_vector(0);
        out = m_vm->eval(A0, av);
    }
    else if (args.m_len == 2)
    {
        if (A1.m_type != T_VEC)
            error("'apply' No argument list given.", A1);
        out = m_vm->eval(A0, A1.m_d.vec);
    }
    else
    {
        if (args.m_data[args.m_len - 1].m_type != T_VEC)
        {
            error("'apply' last argument needs to be a list!",
                  args.m_data[args.m_len - 1]);
        }

        AtomVec *av_last = args.m_data[args.m_len - 1].m_d.vec;
        AtomVec *av =
            m_rt->m_gc.allocate_vector(args.m_len + av_last->m_len);

        for (size_t i = 1; i < args.m_len - 1; i++)
            av->push(args.m_data[i]);

        for (size_t i = 0; i < av_last->m_len; i++)
            av->push(av_last->m_data[i]);

        out = m_vm->eval(A0, av);
    }
END_PRIM_DOC(apply,
"@control procedure (apply _proc_ [_arg1_ ... _argN_] [...])\n"
"\n"
"Calls _proc_ with the elements of the appended lists as argument.\n"
"If there are more arguments than _proc_ then the last argument\n"
"always must be a list.\n"
"\n"
"    (apply str-join \",\" [\"X\" \"Y\"]) ;=> \"X,Y\"\n"
)

START_PRIM()
    REQ_EQ_ARGC(empty?, 1);
    out.m_type = T_BOOL;
    if (A0.m_type == T_VEC)
        out.m_d.b = A0.m_d.vec->m_len == 0;
    else if (A0.m_type == T_MAP)
        out.m_d.b = A0.m_d.map->empty();
    else
        error("Expected map or list to 'empty?'", A0);
END_PRIM_DOC(empty?,
"@lists (empty? _list_)\n"
"\n"
"Returns `#t` if _list_ is an empty list.\n"
"Same as `(null? ...)`.\n"
"\n"
"    (empty?  [1 2 3]) ;=> #f\n"
"    (empty?  '())     ;=> #t\n"
"    (empty?  [])      ;=> #t\n"
"    (empty?  "")      ;=> #t\n"
)

START_PRIM()
    REQ_EQ_ARGC(bkl-gc-statistics, 0);
    m_rt->m_gc.collect();
    out = m_rt->m_gc.get_statistics();
END_PRIM_DOC(bkl-gc-statistics,
"@internal procedure (bkl-gc-statistics)\n"
"\n"
"Performs a garbage collection and returns some statistic values\n"
"in a map.\n"
)

START_PRIM()
    REQ_EQ_ARGC(bkl-primitive-map, 0);
    out = Atom(T_MAP, m_rt->m_gc.allocate_map());
    AtomVec *prim_sym_tbl = m_vm->get_primitive_symbol_table();
    for (size_t i = 0; i < prim_sym_tbl->m_len; i++)
        out.m_d.map->set(prim_sym_tbl->m_data[i], Atom(T_INT, i));
END_PRIM_DOC(bkl-primitive-map,
"@internal procedure (bkl-primitive-map)\n"
"\n"
"Returns a map containing the names of the primitives and their\n"
"indices for the byte code.\n"
)

START_PRIM()
    REQ_EQ_ARGC(bkl-set-doc!, 2);
    m_vm->set_documentation(A0, A1);
    out = A1;
END_PRIM_DOC(bkl-set-doc!,
"@documentation procedure (bkl-set-doc! _func-name-sym-or-kw_ _doc-string_\n"
"\n"
"Stores _doc-string_ in the central documentation library.\n"
"This function is mainly useful if you want to annotate your\n"
"own code with documentation. BukaLISP usually registers everything itself.\n"
"\n"
"See also: `?doc`\n"
)

START_PRIM()
    REQ_EQ_ARGC(list-copy, 1);
    if (A0.m_type != T_VEC)
        error("'list-copy' can only copy lists", A0);
    out.set_vec(m_rt->m_gc.clone_vector(A0.m_d.vec));
END_PRIM_DOC(list-copy,
"@lists procedure (list-copy _list_)\n"
"\n"
"Makes a shallow clone of the _list_.\n"
)

START_PRIM()
    REQ_EQ_ARGC(map-copy, 1);
    if (A0.m_type != T_MAP)
        error("'map-copy' can only copy maps", A0);
    out.set_map(m_rt->m_gc.clone_map(A0.m_d.map));
END_PRIM_DOC(map-copy,
"@maps procedure (map-copy _map_)\n"
"\n"
"Makes a shallow clone of the _map_.\n"
)

START_PRIM()
    REQ_EQ_ARGC(assign, 2);

    if (   A0.m_type != T_MAP
        && A0.m_type != T_VEC)
        error("'assign' can only assign to maps or lists", A0);

    if (A1.m_type == T_MAP)
    {
        if (A0.m_type == T_MAP)
        {
            AtomMap *om = m_rt->m_gc.clone_map(A0.m_d.map);
            ATOM_MAP_FOR(i, A1.m_d.map)
            {
                om->set(MAP_ITER_KEY(i),
                        MAP_ITER_VAL(i));
            }
            out.set_map(om);
        }
        else
        {
            AtomVec *av = m_rt->m_gc.clone_vector(A0.m_d.vec);
            ATOM_MAP_FOR(i, A1.m_d.map)
            {
                if (   MAP_ITER_KEY(i).m_type != T_INT
                    && MAP_ITER_KEY(i).m_type != T_DBL)
                {
                    Atom e = MAP_ITER_KEY(i);
                    error("'assign' bad index in assignment, "
                          "expected int or dbl",
                          e);
                }
                av->set((size_t) MAP_ITER_KEY(i).to_int(), MAP_ITER_VAL(i));
            }
            out.set_vec(av);
        }
    }
    else if (A1.m_type == T_VEC)
    {
        if (A1.m_d.vec->m_len % 2 != 0)
            error("'assign' can only use an assignments list with "
                  "an even number of elements.", A0);

        AtomVec *src_vec = A1.m_d.vec;
        if (A0.m_type == T_MAP)
        {
            AtomMap *om = m_rt->m_gc.clone_map(A0.m_d.map);
            for (size_t i = 0; i < src_vec->m_len; i += 2)
                om->set(src_vec->m_data[i], src_vec->m_data[i + 1]);
            out.set_map(om);
        }
        else
        {
            AtomVec *av = m_rt->m_gc.clone_vector(A0.m_d.vec);
            for (size_t i = 0; i < src_vec->m_len; i += 2)
            {
                if (   src_vec->m_data[i].m_type != T_INT
                    && src_vec->m_data[i].m_type != T_DBL)
                    error("'assign' bad index in assignment, "
                          "expected int or dbl",
                          src_vec->m_data[i]);
                av->set((size_t) src_vec->m_data[i].to_int(),
                        src_vec->m_data[i + 1]);
            }
            out.set_vec(av);
        }
    }
END_PRIM_DOC(assign,
"@maps procedure (assign _destination-map_ _assignments-map_)\n"
"@lists procedure (assign _destination-list_ _assignments-list_)\n"
"\n"
"This procedure creates a clone of _destination-map/-list_\n"
"with the keys defined by the _assignments-map/-list_.\n"
"This is useful if you want to copy and change a list or map at the same time.\n"
"\n"
"    (assign {a: 20 b: 10} {a: 10})\n"
"    ;=> {a: 10 b: 10} where the output map is a shallow clone.\n"
"\n"
"    ; alternative:\n"
"    (assign {a: 20 b: 10} [a: 10])\n"
"\n"
"    (assign [0 1 2 3] [0 10 2 20])\n"
"    ;=> [10 1 20 3] where the output list is a shallow cone.\n"
"\n"
)

START_PRIM()
    REQ_EQ_ARGC(?doc, 1);
    const std::string search = A0.to_display_str();
    if (search.size() <= 0)
        error("'bkl-doc' can't deal with empty search strings!", A0);
    Atom doc = m_vm->get_documentation();
    AtomVec *found = m_rt->m_gc.allocate_vector(0);
    ATOM_MAP_FOR(a, doc.m_d.map)
    {
        if (   MAP_ITER_KEY(a).m_type != T_SYM
            && MAP_ITER_KEY(a).m_type != T_KW)
            continue;
        if (MAP_ITER_VAL(a).m_type != T_STR)
            continue;
        const std::string &funcname = MAP_ITER_KEY(a).m_d.sym->m_str;
        const std::string &docstr   = MAP_ITER_VAL(a).m_d.sym->m_str;

        if (search[0] == '*')
        {
            if (search.size() == 1)
            {
                found->push(MAP_ITER_VAL(a));
            }
            else
            {
                if (   docstr.find(search.substr(1))   != std::string::npos
                    || funcname.find(search.substr(1)) != std::string::npos)
                    found->push(MAP_ITER_VAL(a));
            }
        }
        else if (search[0] == '?')
        {
            size_t p = docstr.find("\n");
            if (p == std::string::npos)
            {
                if (docstr.find(search.substr(1))
                    != std::string::npos)
                    found->push(MAP_ITER_VAL(a));
            }
            else
            {
                if (docstr.substr(0, p).find(search.substr(1))
                    != std::string::npos)
                    found->push(MAP_ITER_VAL(a));
            }
        }
        else
        {
            if (funcname.find(search) != std::string::npos)
                found->push(MAP_ITER_VAL(a));
        }
    }
    out = Atom(T_VEC, found);
END_PRIM_DOC(?doc,
"@documentation procedure (?doc \"*_search-str_\")\n"
"@documentation procedure (?doc \"?_search-str_\")\n"
"@documentation procedure (?doc \"_search-str_\")\n"
"\n"
"This procedure retrieves documentation from the internal documentation\n"
"database. It returns a list of all matching documentation strings\n"
"\n"
"If _search-str_ starts with a '*' the key (function name) and\n"
"the documentation string for that key will be checked if they contain\n"
"_search-str_.\n"
"If _search-str_ consists only of '*' all documentation entries will be returned,\n"
"which is usually only useful if you want to write your own documentation formatter.\n"
"If _search-str_ starts with a '?' only the first line in the documentation\n"
"will be checked if it contains _search-str_.\n"
"Otherwise only the keys in the database will be checked whether they\n"
"contain _search-str.\n"
"\n"
"Instead of strings you can also use symbols or keywords as search string,"
"like this:\n"
"\n"
"    (?doc ?doc:) ;=> [\"@documentation procedure (?doc ...\"];  \n"
"\n"
"\n"
"See also: `bkl-set-doc!`\n"
)

START_PRIM()
    REQ_EQ_ARGC(bkl-find-lib-file, 1);
    REQ_S_ARG(A0,
        "'bkl-find-lib-file' requires a string, symbol "
        "or keyword as first argument.");
    out =
        Atom(T_STR,
            m_rt->m_gc.new_symbol(
                m_rt->find_in_libdirs(A0.m_d.sym->m_str)));
END_PRIM_DOC(bkl-find-lib-file,
"@runtime procedure (bkl-find-lib-file _string_)\n\n"
"Tries to find the filename (or sub path) _string_ in the\n"
"lib paths of `bklisp`.\n"
"If the file was not found in any lib directory, the empty\n"
"string is returned. Otherwise the full path to that file is returned.\n"
"Standard lib directory paths are in order:\n"
"\n"
"    ./bukalisplib/\n"
"    $ENV{BUKALISP_LIB}/\n"
"    <path-to-bklisp-binary>/\n"
"    <path-to-bklisp-binary>/bukalisplib/\n")

START_PRIM()
    REQ_EQ_ARGC(file-exists?, 1);
    REQ_S_ARG(A0,
        "'file-exists' requires a string, symbol "
        "or keyword as first argument.");
    out.set_bool(file_exists(A0.m_d.sym->m_str));
END_PRIM_DOC(file-exists?,
"@sys procedure (file-exists? _string_)\n\n"
"Checks if the file _string_ is readable and existent.\n")

#include "port_primitives.cpp"

#if IN_INTERPRETER

START_PRIM()
    REQ_GT_ARGC(eval, 1);
    std::cout << "I-EVAL CALLED" << std::endl;
    if (args.m_len > 1)
    {
        if (A1.m_type != T_MAP)
            error("Can invoke 'eval' only with a map as env", A1);
        out = eval(A0, A1.m_d.map);
    }
    else
        out = eval(A0);
END_PRIM(eval)

START_PRIM()
    REQ_EQ_ARGC(interaction-environment, 0);
    out = Atom(T_MAP, init_root_env());
END_PRIM(interaction-environment)

// (invoke-compiler code-name code only-compile root-env)
// (invoke-compiler [code] code-name only-compile root-env)
START_PRIM()
    REQ_GT_ARGC(invoke-compiler, 4);
    if (A3.m_type != T_MAP)
        error("invoke-compiler needs a root-env map "
              "to compile and evaluate in", A3);

    if (A0.m_type == T_STR)
        out =
            this->call_compiler(
                A1.to_display_str(), A0.to_display_str(),
                A3.m_d.map, !A2.is_false());
    else
        out =
            this->call_compiler(
                A0, A3.m_d.map, A1.to_display_str(), !A2.is_false());
END_PRIM(invoke-compiler)

#else

START_PRIM()
    REQ_GT_ARGC(eval, 1);
    std::cout << "VM-EVAL CALLED" << std::endl;

    Atom exprs(T_VEC, m_rt->m_gc.allocate_vector(1));
    exprs.m_d.vec->push(A0);

    AtomMap *env = nullptr;
    if (args.m_len > 1)
    {
        if (A1.m_type != T_MAP)
            error("Can invoke 'eval' only with a vec as env", A1);
        env = A1.m_d.map;
    }
    else
    {
        env = m_rt->m_gc.allocate_map();
    }

    Atom prog =
        m_compiler_call(exprs, env, "eval", true);
    out = eval(prog, nullptr);
END_PRIM(eval)

START_PRIM()
    REQ_EQ_ARGC(interaction-environment, 0);
    GC_ROOT_MAP(m_rt->m_gc, root_env) = m_rt->m_gc.allocate_map();
    m_compiler_call(Atom(T_INT, 42), root_env, "env-gen", true);
    out = Atom(T_MAP, root_env);
END_PRIM(interaction-environment)

START_PRIM()
    REQ_GT_ARGC(invoke-compiler, 4);
    if (A3.m_type != T_MAP)
        error("invoke-compiler needs a root-env (<map>, <storage>) "
              "to compile and evaluate in", A3);

    if (A0.m_type == T_STR)
    {
        GC_ROOT(m_rt->m_gc, input) =
            m_rt->read(A1.to_display_str(), A0.to_display_str());
        out =
            m_compiler_call(
                input, A3.m_d.map, A1.to_display_str(), !A2.is_false());
    }
    else
        out =
            m_compiler_call(
                A0, A3.m_d.map, A1.to_display_str(), !A2.is_false());
END_PRIM(invoke-compiler)


#endif

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
