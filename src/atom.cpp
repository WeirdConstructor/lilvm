#include "atom.h"

namespace lilvm
{
//---------------------------------------------------------------------------

size_t AtomVec::s_alloc_count = 0;

//---------------------------------------------------------------------------

size_t AtomHash::operator()(const Atom &a) const
{
    // TODO: use X macro
    switch (a.m_type)
    {
        case T_NIL:
        case T_INT:
            return std::hash<int64_t>()(a.m_d.i);

        case T_BOOL:
            return std::hash<bool>()(a.m_d.b);

        case T_DBL:  return std::hash<double>()(a.m_d.d);

        case T_PRIM: return std::hash<void *>()((void *) a.m_d.func);

        case T_VEC:
        case T_CLOS:
            return std::hash<void *>()((void *) a.m_d.vec);

        case T_MAP:
            return std::hash<void *>()((void *) a.m_d.map);

        case T_SYNTAX:
        case T_SYM:
        case T_KW:
        case T_STR:
            return std::hash<std::string>()(a.m_d.sym->m_str);
    }

    return std::hash<void *>()((void *) a.m_d.vec);
}
//---------------------------------------------------------------------------

void AtomVec::alloc(size_t len)
{
    m_alloc = len;
    m_len   = 0;
    m_data  = new Atom[len];
}
//---------------------------------------------------------------------------

void AtomVec::init(uint8_t current_gc_color, size_t len)
{
    m_len      = len;
    m_gc_color = current_gc_color;

    for (size_t i = 0; i < len; i++)
        m_data[i].clear();
}
//---------------------------------------------------------------------------

Atom AtomVec::at(size_t idx)
{
    if (idx > m_len)
        return Atom();
    return m_data[idx];
}
//---------------------------------------------------------------------------

Atom *AtomVec::first()
{
    if (m_len <= 0) return nullptr;
    return m_data;
}
//---------------------------------------------------------------------------

Atom *AtomVec::last()
{
    if (m_len <= 0) return nullptr;
    return &(m_data[m_len - 1]);
}
//---------------------------------------------------------------------------

void AtomVec::pop()
{
    if (m_len > 0) m_len--;
}
//---------------------------------------------------------------------------

void AtomVec::push(const Atom &a)
{
    size_t new_idx = m_len;
    check_size(new_idx);
    m_data[new_idx] = a;
}
//---------------------------------------------------------------------------

void AtomVec::check_size(size_t idx)
{
    if (idx >= m_alloc)
    {
        Atom *old_data = m_data;
        m_alloc = idx * 2;
        m_data = new Atom[m_alloc];
        for (size_t i = 0; i < m_len; i++)
            m_data[i] = old_data[i];
        delete[] old_data;
    }

    m_len = idx + 1;
}
//---------------------------------------------------------------------------

AtomVec::~AtomVec()
{
    s_alloc_count--;

    if (m_data)
        delete[] m_data;
}
//---------------------------------------------------------------------------

void AtomMap::set(Sym *s, Atom &a)
{
    Atom sym(T_SYM, s);
    m_map[sym] = a;
}
//---------------------------------------------------------------------------

void AtomMap::set(const Atom &k, Atom &a)
{
    m_map[k] = a;
}
//---------------------------------------------------------------------------

Atom AtomMap::at(const Atom &k)
{
    auto it = m_map.find(k);
    if (it == m_map.end()) return Atom();
    return it->second;
}
//---------------------------------------------------------------------------

Atom AtomMap::at(const Atom &k, bool &defined)
{
    defined = false;
    auto it = m_map.find(k);
    if (it == m_map.end()) return Atom();
    defined = true;
    return it->second;
}
//---------------------------------------------------------------------------

};
