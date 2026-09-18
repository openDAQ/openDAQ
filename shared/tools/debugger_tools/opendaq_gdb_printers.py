import gdb #type: ignore
import gdb.printing #type: ignore
import re

string_impl_type = None
field_cache = {}
_vtable_type_cache = {}
_void_pp_type = None
_MAX_STRING_LENGTH = 4096
_MAX_EXPANSION_DEPTH = 2
_current_depth = 0


def set_max_expansion_depth(n):
    global _MAX_EXPANSION_DEPTH
    _MAX_EXPANSION_DEPTH = max(0, int(n))

_CORETYPE_NAMES = {
    0: "Bool",
    1: "Int",
    2: "Float",
    3: "String",
    4: "List",
    5: "Dict",
    6: "Ratio",
    7: "Proc",
    8: "Object",
    9: "BinaryData",
    10: "Func",
    11: "ComplexNumber",
    12: "Struct",
    13: "Enumeration",
    0xFFFF: "Undefined",
}


def find_field(value, field_name, max_depth=10):
    cache_key = (str(value.type), field_name)
    if cache_key in field_cache and field_cache[cache_key] is None:
        return None  
    try:
        return value[field_name]
    except Exception:
        pass
    
    if max_depth > 0:
        try:
            type_info = value.type.strip_typedefs()
            for field in type_info.fields():
                if field.is_base_class:
                    result = find_field(value[field], field_name, max_depth - 1)
                    if result is not None:
                        return result
        except Exception:
            pass
    field_cache[cache_key] = None
    return None


def read_string_safe(pointer):
    global string_impl_type
    try:
        if string_impl_type is None:
            string_impl_type = gdb.lookup_type("daq::StringImpl").pointer()
        str_impl = pointer.cast(string_impl_type).dereference()
        char_ptr = str_impl["str"]
        if int(char_ptr) == 0:
            return ""
        str_length = int(str_impl["length"])
        if str_length <= 0 or str_length > _MAX_STRING_LENGTH:
            return char_ptr.string()
        return char_ptr.string(length=str_length)
    except Exception:
        return None


def read_atomic_int(value):
    for field_name in ("_M_i", "_My_val"):
        try:
            return int(value[field_name])
        except Exception:
            pass
    try:
        return int(value)
    except Exception:
        return None


def get_smart_ptr_raw(value):
    try:
        raw = value["object"]
    except Exception:
        return None

    try:
        if int(raw) == 0:
            return raw
    except Exception:
        return raw

    try:
        type_name = str(value.type.strip_typedefs())
    except Exception:
        return raw

    if "WeakRefPtr" not in type_name:
        return raw

    try:
        weak_impl = resolve_impl(raw)
        ref_count = find_field(weak_impl, "refCount")
        if ref_count is not None:
            strong = read_atomic_int(ref_count["strong"])
            if strong == 0:
                return None

        target = find_field(weak_impl, "object")
        if target is None:
            return None
        if int(target) == 0:
            return None
        return target
    except Exception:
        return None


def is_string_type(type_name):
    return "String" in type_name or "string" in type_name


def is_property_type(type_name):
    if "Property" not in type_name:
        return False
    if "PropertyObject" in type_name or "PropertyValue" in type_name:
        return False
    return True


_dynamic_type_by_addr = {}


def get_dynamic_type(pointer):
    global _void_pp_type
    try:
        ptr_addr = int(pointer)
    except Exception:
        ptr_addr = None

    if ptr_addr is not None:
        cached = _dynamic_type_by_addr.get(ptr_addr)
        if cached is not None:
            return cached

    try:
        if _void_pp_type is None:
            _void_pp_type = gdb.lookup_type("void").pointer().pointer()
        vtable_addr = int(pointer.cast(_void_pp_type).dereference())
    except Exception:
        dynamic_type = pointer.dynamic_type
        if ptr_addr is not None:
            _dynamic_type_by_addr[ptr_addr] = dynamic_type
        return dynamic_type

    if vtable_addr in _vtable_type_cache:
        dynamic_type = _vtable_type_cache[vtable_addr]
    else:
        dynamic_type = pointer.dynamic_type
        _vtable_type_cache[vtable_addr] = dynamic_type

    if ptr_addr is not None:
        _dynamic_type_by_addr[ptr_addr] = dynamic_type
    return dynamic_type


def collect_field_values(value, type_info=None, depth=0, visited=None):
    if visited is None:
        visited = set()
    if depth > 15:
        return {}

    if type_info is None:
        type_info = value.type

    try:
        t = type_info.strip_typedefs()
    except Exception:
        return {}

    key = str(t)
    if key in visited:
        return {}
    visited.add(key)

    result = {}
    try:
        for field in t.fields():
            try:
                if field.is_base_class:
                    base_fields = collect_field_values(
                        value[field], field.type, depth + 1, visited)
                    for name, val in base_fields.items():
                        if name not in result:
                            result[name] = val
                elif field.name and field.name not in result:
                    result[field.name] = value[field.name]
            except Exception:
                pass
    except Exception:
        pass
    return result


def resolve_impl(pointer):
    dynamic_type = get_dynamic_type(pointer).strip_typedefs()

    if dynamic_type.code == gdb.TYPE_CODE_PTR:
        pointer_type = dynamic_type
    else:
        pointer_type = dynamic_type.pointer()

    return pointer.cast(pointer_type).dereference()


_TYPE_PATTERNS = [
    ("BoolImpl", "scalar", "Bool"),
    ("BooleanImpl", "scalar", "Bool"),
    ("IBoolean", "scalar", "Bool"),
    ("StringImpl", "scalar", "String"),
    ("ComplexNumber", "scalar", "ComplexNumber"),
    ("RatioImpl", "scalar", "Ratio"),
    ("FloatImpl", "scalar", "Float"),
    ("IFloat", "scalar", "Float"),
    ("IntegerImpl", "scalar", "Int"),
    ("IInteger", "scalar", "Int"),
    ("NumberImpl", "scalar", "Number"),
    ("OrdinalObject", "scalar", "Number"),
    ("BinaryDataImpl", "binary", "BinaryData"),
    ("EnumerationImpl", "enumeration", "Enumeration"),
    ("StructImpl", "struct", "Struct"),
    ("GenericStructImpl", "struct", "Struct"),
    ("ListImpl", "list", "List"),
    ("DictImpl", "dict", "Dict"),
]


def classify_type(impl_type):
    for pattern, category, type_name in _TYPE_PATTERNS:
        if pattern in impl_type:
            return (category, type_name)
    return (None, None)


_string_storage = {}
_leaf_storage = {}
_synthetic_storage = {}
_synthetic_children_cache = {}
_group_id_counter = 0
_void_p_type_cached = None
_ull_type_cached = None


def _alloc_void_ptr(gid):
    global _void_p_type_cached
    try:
        if _void_p_type_cached is None:
            _void_p_type_cached = gdb.lookup_type("void").pointer()
        return gdb.Value(gid).cast(_void_p_type_cached)
    except Exception:
        return gdb.parse_and_eval("(void*){:d}".format(gid))


def _alloc_ull(gid):
    global _ull_type_cached
    try:
        if _ull_type_cached is None:
            _ull_type_cached = gdb.lookup_type("unsigned long long")
        return gdb.Value(gid).cast(_ull_type_cached)
    except Exception:
        return gdb.parse_and_eval("(unsigned long long){:d}".format(gid))


def _next_id():
    global _group_id_counter
    _group_id_counter += 1
    return _group_id_counter


def make_leaf_value(text):
    gid = _next_id()
    _leaf_storage[gid] = text
    val = _alloc_ull(gid)
    return val


def make_string_value(text):
    gid = _next_id()
    _string_storage[gid] = text
    val = _alloc_ull(gid)
    return val


def make_value_with_children(text, children_fn):
    gid = _next_id()
    _synthetic_storage[gid] = (gid, text, children_fn, _current_depth)
    val = _alloc_void_ptr(gid)
    return val


def make_group(children_or_fn, count_hint=None):
    if isinstance(children_or_fn, list):
        text = "{{{} items}}".format(len(children_or_fn))
    elif count_hint is not None:
        text = "{{{} items}}".format(count_hint)
    else:
        text = "{...}"
    gid = _next_id()
    _synthetic_storage[gid] = (gid, text, children_or_fn, _current_depth)
    return _alloc_void_ptr(gid)


class LeafPrinter:
    def __init__(self, text):
        self._text = text

    def to_string(self):
        return self._text
    

class StringPrinter:
    def __init__(self, text):
        self._text = text

    def display_hint(self):
        return "string" 

    def to_string(self):
        return self._text


class SyntheticPrinter:
    def __init__(self, gid, text, source, depth=0):
        self._gid = gid
        self._text = text
        self._source = source
        self._depth = depth

    def to_string(self):
        return self._text

    def children(self):
        global _current_depth
        cached = _synthetic_children_cache.get(self._gid)
        if cached is None:
            saved = _current_depth
            _current_depth = 1
            try:
                if callable(self._source):
                    cached = self._source()
                else:
                    cached = self._source
            finally:
                _current_depth = saved
            _synthetic_children_cache[self._gid] = cached
        for name, val in cached:
            yield (name, val)


def iter_deque(deque_val):
    try:
        printer = gdb.default_visualizer(deque_val)
        if printer is not None:
            for name, val in printer.children():
                yield val
            return
    except Exception:
        pass

    try:
        impl = deque_val["_M_impl"]
        start = impl["_M_start"]
        finish = impl["_M_finish"]

        s_cur = start["_M_cur"]
        s_last = start["_M_last"]
        s_node = start["_M_node"]

        f_cur = finish["_M_cur"]
        f_node = finish["_M_node"]

        elem_type = s_cur.type.target()
        elem_size = elem_type.sizeof
        buf_size = max(1, 512 // elem_size)

        node = s_node

        if int(node) == int(f_node):
            cur = s_cur
            while int(cur) != int(f_cur):
                yield cur.dereference()
                cur += 1
        else:
            cur = s_cur
            while int(cur) != int(s_last):
                yield cur.dereference()
                cur += 1

            node += 1
            while int(node) != int(f_node):
                buf_start = node.dereference()
                for i in range(buf_size):
                    yield (buf_start + i).dereference()
                node += 1

            buf_start = node.dereference()
            cur = buf_start
            while int(cur) != int(f_cur):
                yield cur.dereference()
                cur += 1
    except Exception:
        return


def iter_dict(dict_impl):
    try:
        ht = find_field(dict_impl, "hashTable")
        if ht is None:
            return
        m_ht = ht["m_ht"]
        m_values = m_ht["m_values"]
        for pair in iter_deque(m_values):
            yield (pair["first"], pair["second"])
    except Exception:
        return


def iter_local_properties(impl):
    try:
        local_props = find_field(impl, "localProperties")
        if local_props is None:
            return
        m_ht = local_props["m_ht"]
        m_values = m_ht["m_values"]

        for pair in iter_deque(m_values):
            try:
                key_obj = pair["first"]
                prop_ptr = pair["second"]

                key_raw = key_obj["object"]
                if int(key_raw) == 0:
                    continue
                prop_name = read_string_safe(key_raw)
                yield (prop_name, prop_ptr)
            except Exception:
                continue
    except Exception:
        return


def iter_ordered_map(ordered_map_val):
    try:
        m_ht = ordered_map_val["m_ht"]
        m_values = m_ht["m_values"]
        for pair in iter_deque(m_values):
            yield pair
    except Exception:
        return


def iter_class_properties(impl, visited=None):
    if visited is None:
        visited = set()
    try:
        obj_class = find_field(impl, "objectClass")
        if obj_class is None:
            return
        class_raw = obj_class["object"]
        if int(class_raw) == 0:
            return
        class_impl = resolve_impl(class_raw)
        class_type = str(class_impl.type.strip_typedefs())
        if class_type in visited:
            return
        visited.add(class_type)

        props = find_field(class_impl, "props")
        if props is not None:
            for pair in iter_ordered_map(props):
                try:
                    key_obj = pair["first"]
                    prop_ptr = pair["second"]
                    key_raw = key_obj["object"]
                    if int(key_raw) == 0:
                        continue
                    prop_name = read_string_safe(key_raw)
                    yield (prop_name, prop_ptr)
                except Exception:
                    continue
    except Exception:
        return


def get_all_properties(impl):
    seen = {}
    for name, prop in iter_class_properties(impl):
        seen[name] = prop

    for name, prop in iter_local_properties(impl):
        seen[name] = prop

    for name, prop in seen.items():
        yield (name, prop)



_HIDDEN_FIELDS = {
    "Id", "GuidSource", "disposeCalled", "DisposeCalled",
    "_vptr.IUnknown", "refAdded", "sync", "cv", "itemId", "iid", "defaultComponents",
    "keyId", "valueId", "refCount",
}

_COMPONENT_ATTRIBUTE_FIELDS = [
    ("active",           "Active"),
    ("name",             "Name"),
    ("description",      "Description"),
    ("tags",             "Tags"),
    ("visible",          "Visible"),
    ("statusContainer",  "Status Container"),
    ("parent",           "Parent"),
]

_COMPONENT_KNOWN_FIELDS = {f for f, _ in _COMPONENT_ATTRIBUTE_FIELDS}
_COMPONENT_KNOWN_FIELDS.update({
    "globalId", "localId",
    "localProperties", "propValues", "className", "objectClass",
    "valueWriteEvents", "valueReadEvents",
    "sync", "lockOwner", "lockingStrategy", "owner",
    "updateCount", "updatingPropsAndValues", "updatePropertyStack",
    "propObjCore", "triggerCoreEvent", "customOrder", "path",
    "permissionManager", "frozen",
})

_component_type_cache = {}
_hierarchy_keyword_cache = {}


def is_component(type_key, impl):
    if type_key in _component_type_cache:
        return _component_type_cache[type_key]
    result = (find_field(impl, "globalId") is not None and
              find_field(impl, "localId") is not None)
    _component_type_cache[type_key] = result
    return result


def _hierarchy_contains(gdb_type, keyword, _visited=None):
    if _visited is None:
        _visited = set()
    try:
        t = gdb_type.strip_typedefs()
        tid = str(t)
        if tid in _visited:
            return False
        _visited.add(tid)
        if keyword in tid:
            return True
        for field in t.fields():
            if field.is_base_class:
                if _hierarchy_contains(field.type, keyword, _visited):
                    return True
    except Exception:
        pass
    return False


def _type_has(type_key, impl_type, keyword):
    cache_key = (type_key, keyword)
    if cache_key in _hierarchy_keyword_cache:
        return _hierarchy_keyword_cache[cache_key]
    result = _hierarchy_contains(impl_type, keyword)
    _hierarchy_keyword_cache[cache_key] = result
    return result



def _get_type_name_from_pointer(raw_ptr):
    if int(raw_ptr) == 0:
        return None
    try:
        impl = resolve_impl(raw_ptr)
        impl_type = str(impl.type.strip_typedefs())
        _, type_name = classify_type(impl_type)
        if type_name:
            return type_name
        if read_string_safe(raw_ptr) is not None:
            return "String"
    except Exception:
        pass
    return None


def get_unit_symbol(unit_ptr_field):
    try:
        unit_raw = unit_ptr_field["object"]
        if int(unit_raw) == 0:
            return None
        unit_impl = resolve_impl(unit_raw)
        fields_field = find_field(unit_impl, "fields")
        if fields_field is None:
            return None
        fields_raw = fields_field["object"]
        if int(fields_raw) == 0:
            return None
        dict_impl = resolve_impl(fields_raw)
        for key_ptr, val_ptr in iter_dict(dict_impl):
            if int(key_ptr) == 0:
                continue
            key_str = read_string_safe(key_ptr)
            if key_str == "Symbol":
                if int(val_ptr) == 0:
                    return None
                return read_string_safe(val_ptr)
    except Exception:
        return None
    return None


def get_property_summary(raw_pointer):
    try:
        impl = resolve_impl(raw_pointer)
        impl_type = str(impl.type.strip_typedefs())
        if "PropertyImpl" not in impl_type:
            return None

        parts = []

        name_field = find_field(impl, "name")
        if name_field is not None:
            name_raw = name_field["object"]
            if int(name_raw) != 0:
                parts.append('"{}"'.format(read_string_safe(name_raw)))

        vt = find_field(impl, "valueType")
        if vt is not None:
            vt_int = int(vt)
            parts.append("[{}]".format(_CORETYPE_NAMES.get(vt_int, "?")))

        unit_field = find_field(impl, "unit")
        if unit_field is not None:
            sym = get_unit_symbol(unit_field)
            if sym:
                parts.append("({})".format(sym))

        if parts:
            return " ".join(parts)
    except Exception:
        pass
    return None


def make_child(name, field_val):
    try:
        type_str = field_val.type.strip_typedefs()
        type_name = str(type_str)
        code = type_str.code
    except Exception:
        return None

    if "daq::" in type_name and "Ptr" in type_name:
        try:
            obj = get_smart_ptr_raw(field_val)
            if obj is None:
                return None
            if int(obj) == 0:
                return None
            return (name, _make_node_value(obj))
        except Exception:
            return None

    if code == gdb.TYPE_CODE_PTR:
        if int(field_val) == 0:
            return None
        return (name, field_val)

    if "string" in type_name.lower() and "std::" in type_name:
        try:
            vis = gdb.default_visualizer(field_val)
            if vis is not None:
                s = vis.to_string()
                if s in ('""', '', '\"\"'):
                    return None
                return (name, make_string_value(s))
            return (name, field_val)
        except Exception:
            return (name, field_val)

    if any(c in type_name for c in ("vector", "deque", "set", "map", "list",
                                       "unordered_set", "unordered_map")):
        try:
            vis = gdb.default_visualizer(field_val)
            if vis is not None:
                s = vis.to_string()
                if s and ("0 elements" in s or "length 0" in s):
                    return None
                first_child = next(iter(vis.children()), None)
                if first_child is None:
                    return None
            return (name, field_val)
        except Exception:
            return (name, field_val)

    if code in (gdb.TYPE_CODE_INT, gdb.TYPE_CODE_BOOL, gdb.TYPE_CODE_FLT,
                gdb.TYPE_CODE_ENUM, gdb.TYPE_CODE_CHAR):
        return (name, field_val)

    if "atomic" in type_name:
        try:
            inner = field_val["_M_i"]
            return (name, inner)
        except Exception:
            return (name, field_val)

    try:
        vis = gdb.default_visualizer(field_val)
        if vis is not None:
            s = vis.to_string()
            if s and ("0 elements" in s or "length 0" in s):
                return None
    except Exception:
        pass

    return (name, field_val)


def get_generic_children(impl):
    all_fields = collect_field_values(impl)
    for n, field_val in all_fields.items():
        if n in _HIDDEN_FIELDS:
            continue
        if n and n.startswith("on"):
            continue
        child = make_child(n, field_val)
        if child is not None:
            yield child

#MARK: CLASSES

class BaseNode:

    def __init__(self, impl, raw_pointer):
        self.impl = impl
        self.raw_pointer = raw_pointer
        self._all_fields_cache = None


    def get_type_name(self):
        return "Object"

    def get_display_value(self):
        return None


    def to_string(self):
        val = self.get_display_value()
        tn = self.get_type_name()
        if val is not None:
            return "{} [{}]".format(val, tn)
        return "[{}]".format(tn)


    def _get_all_fields(self):
        if self._all_fields_cache is None:
            self._all_fields_cache = collect_field_values(self.impl)
        return self._all_fields_cache


    def _yield_refcount(self):
        ref_count = find_field(self.impl, "refCount")
        if ref_count is None:
            return
        try:
            yield ("RefCount", ref_count["_M_i"])
            return
        except Exception:
            pass
        try:
            vis = gdb.default_visualizer(ref_count)
            inner_ptr = None
            if vis is not None:
                for child_name, child_val in vis.children():
                    if child_name == "get()":
                        inner_ptr = child_val
                        break
            if inner_ptr is not None and int(inner_ptr) != 0:
                ref_count_obj = inner_ptr.dereference()
                strong = int(ref_count_obj["strong"]["_M_i"])
                weak = int(ref_count_obj["weak"]["_M_i"])
                yield ("RefCount", make_leaf_value("[S:{}, W:{}]".format(strong, weak)))
                return
        except Exception:
            pass
        yield ("RefCount", ref_count)

    def _yield_address(self):
        ret = (make_leaf_value("0x{:x}".format(int(self.raw_pointer))))
        yield ("Address", ret)

    def _yield_type(self):
        ret = ( make_leaf_value(self.get_type_name()) )
        yield ("Type", ret)

    def _yield_raw(self):
        def make_raw_lazy():
            return [(n, v) for n, v in self._get_all_fields().items()]
        yield ("Raw", make_group(make_raw_lazy))

    def _yield_type_specific(self):
        return
        yield  

    def children(self):
        yield from self._yield_refcount()
        yield from self._yield_address()
        yield from self._yield_type()
        yield from self._yield_type_specific()
        yield from self._yield_raw()



class ScalarNode(BaseNode):

    def __init__(self, impl, raw_pointer, type_name):
        super().__init__(impl, raw_pointer)
        self._type_name = type_name

    def get_type_name(self):
        return self._type_name

    def get_display_value(self):
        if self._type_name == "String":
            s = read_string_safe(self.raw_pointer)
            if s is not None:
                return '"{}"'.format(s)
            return None

        if self._type_name == "ComplexNumber":
            val = find_field(self.impl, "value")
            if val is not None:
                try:
                    real = float(val["real"])
                    imag = float(val["imaginary"])
                    if imag >= 0:
                        return "{} + {}i".format(real, imag)
                    return "{} - {}i".format(real, abs(imag))
                except Exception:
                    pass
            return None

        if self._type_name == "Ratio":
            num = find_field(self.impl, "numerator")
            den = find_field(self.impl, "denominator")
            if num is not None and den is not None:
                return "{}/{}".format(int(num), int(den))
            return None

        val = find_field(self.impl, "value")
        if val is not None:
            if self._type_name == "Bool":
                return "true" if int(val) != 0 else "false"
            if val.type.strip_typedefs().code == gdb.TYPE_CODE_FLT:
                return str(float(val))
            return str(int(val))
        return None

    def _yield_type_specific(self):
        if self._type_name in ("Int", "Number"):
            val = find_field(self.impl, "value")
            if val is not None:
                yield ("Value", val)
            return

        if self._type_name == "Float":
            val = find_field(self.impl, "value")
            if val is not None:
                yield ("Value", val)
            return

        if self._type_name == "Bool":
            val = find_field(self.impl, "value")
            if val is not None:
                yield ("Value", val)
            return

        if self._type_name == "Ratio":
            num = find_field(self.impl, "numerator")
            den = find_field(self.impl, "denominator")
            if num is not None and den is not None:
                yield ("Numerator", num)
                yield ("Denominator", den)
            return

        display = self.get_display_value()
        if display is not None:
            yield ("Value", make_leaf_value(display))

class BinaryNode(BaseNode):

    def get_type_name(self):
        return "BinaryData"

    def get_display_value(self):
        size_field = find_field(self.impl, "size")
        if size_field is not None:
            return "{} bytes".format(int(size_field))
        return None

    def _yield_type_specific(self):
        size_field = find_field(self.impl, "size")
        if size_field is not None:
            yield ("Size", size_field)


class EnumerationNode(BaseNode):

    def get_type_name(self):
        try:
            et = find_field(self.impl, "enumerationType")
            if et is not None:
                et_raw = et["object"]
                if int(et_raw) != 0:
                    et_impl = resolve_impl(et_raw)
                    tn = find_field(et_impl, "typeName")
                    if tn is not None:
                        tn_raw = tn["object"]
                        if int(tn_raw) != 0:
                            n = read_string_safe(tn_raw)
                            if n:
                                return n
        except Exception:
            pass
        return "Enumeration"

    def get_display_value(self):
        value_field = find_field(self.impl, "value")
        if value_field is not None:
            try:
                val_raw = value_field["object"]
                if int(val_raw) != 0:
                    s = read_string_safe(val_raw)
                    if s is not None:
                        return '"{}"'.format(s)
            except Exception:
                pass
        return None

    def _yield_type_specific(self):
        display = self.get_display_value()
        if display is not None:
            yield ("Value", make_leaf_value(display))


class ListNode(BaseNode):

    def __init__(self, impl, raw_pointer):
        super().__init__(impl, raw_pointer)
        self._item_type = None
        self._count = 0
        self._vis = None
        self._setup()

    def _setup(self):
        list_field = find_field(self.impl, "list")
        if list_field is None:
            return
        try:
            self._vis = gdb.default_visualizer(list_field)
            if self._vis is None:
                return
            s = self._vis.to_string()
            if s:
                m = re.search(r'(\d+)', s)
                if m:
                    self._count = int(m.group(1))
            if self._count > 0:
                first = next(iter(self._vis.children()), None)
                if first is not None:
                    _, first_ptr = first
                    self._item_type = _get_type_name_from_pointer(first_ptr)
        except Exception:
            pass

    def get_type_name(self):
        if self._item_type:
            return "List<{}>".format(self._item_type)
        return "List"

    def _yield_type_specific(self):
        if self._vis is not None:
            def make_list_items(v=self._vis):
                return [("[{}]".format(idx), _make_node_value(item))
                        for idx, (_, item) in enumerate(v.children())]
            yield ("Items", make_group(make_list_items,
                                      count_hint=self._count))


class DictNode(BaseNode):

    def __init__(self, impl, raw_pointer):
        super().__init__(impl, raw_pointer)
        self._entries = None
        self._key_type = None
        self._value_type = None
        self._setup()

    def _setup(self):
        ht = find_field(self.impl, "hashTable")
        if ht is None:
            return
        try:
            self._entries = list(iter_dict(self.impl))
            if self._entries:
                kp0, vp0 = self._entries[0]
                if int(kp0) != 0:
                    self._key_type = _get_type_name_from_pointer(kp0)
                if int(vp0) != 0:
                    self._value_type = _get_type_name_from_pointer(vp0)
        except Exception:
            self._entries = []

    def get_type_name(self):
        if self._key_type or self._value_type:
            return "Dict<{}, {}>".format(
                self._key_type or "?", self._value_type or "?")
        return "Dict"

    def _yield_type_specific(self):
        if self._entries:
            def make_dict_items(ents=self._entries):
                result = []
                for idx, (kp, vp) in enumerate(ents):
                    def make_entry(k=kp, v=vp):
                        return [("Key", _make_node_value(k)),
                                ("Value", _make_node_value(v))]
                    result.append(("[{}]".format(idx),
                                   make_group(make_entry)))
                return result
            yield ("Items", make_group(make_dict_items,
                                      count_hint=len(self._entries)))


class StatusContainerNode(BaseNode):

    def get_type_name(self):
        return "StatusContainer"

    def get_display_value(self):
        statuses_field = find_field(self.impl, "statuses")
        if statuses_field is None:
            return None
        try:
            status_raw = get_smart_ptr_raw(statuses_field)
            if status_raw is None or int(status_raw) == 0:
                return None
            status_impl = resolve_impl(status_raw)
            count = sum(1 for _ in iter_dict(status_impl))
            return "{} {}".format(count, "status" if count == 1 else "statuses")
        except Exception:
            return None

    def _iter_statuses(self):
        msg_map = {}
        messages_field = find_field(self.impl, "messages")
        if messages_field is not None:
            try:
                msg_raw = get_smart_ptr_raw(messages_field)
                if msg_raw is not None and int(msg_raw) != 0:
                    msg_impl = resolve_impl(msg_raw)
                    for key_ptr, val_ptr in iter_dict(msg_impl):
                        if int(key_ptr) == 0:
                            continue
                        key_str = read_string_safe(key_ptr) 
                        if key_str is not None:
                            msg_map[key_str] = val_ptr
            except Exception:
                pass

        statuses_field = find_field(self.impl, "statuses")
        if statuses_field is None:
            return
        try:
            status_raw = get_smart_ptr_raw(statuses_field)
            if status_raw is None or int(status_raw) == 0:
                return
            status_impl = resolve_impl(status_raw)
            for key_ptr, val_ptr in iter_dict(status_impl):
                if int(key_ptr) == 0:
                    continue
                key_str = read_string_safe(key_ptr)
                if key_str is None:
                    continue
                yield (key_str, val_ptr, msg_map.get(key_str))
        except Exception:
            return

    def _yield_type_specific(self):
        entries = list(self._iter_statuses())
        if not entries:
            return

        def make_items(ents=entries):
            result = []
            for name, status_ptr, msg_ptr in ents:
                def make_entry(s=status_ptr, m=msg_ptr):
                    children = [("Value", _make_node_value(s))]
                    if m is not None:
                        try:
                            if int(m) != 0:
                                children.append(("Message", _make_node_value(m)))
                        except Exception:
                            pass
                    return children
                result.append((name, make_group(make_entry)))
            return result

        yield ("Statuses", make_group(make_items, count_hint=len(entries)))


class StructNode(BaseNode):

    def get_type_name(self):
        try:
            struct_type_field = find_field(self.impl, "structType")
            if struct_type_field is not None:
                struct_type_raw = struct_type_field["object"]
                if int(struct_type_raw) != 0:
                    st_impl = resolve_impl(struct_type_raw)
                    tn_field = find_field(st_impl, "typeName")
                    if tn_field is not None:
                        tn_raw = tn_field["object"]
                        if int(tn_raw) != 0:
                            s = read_string_safe(tn_raw)
                            if s:
                                return s
        except Exception:
            pass
        return "Struct"

    def _yield_type(self):
        struct_type_field = find_field(self.impl, "structType")
        if struct_type_field is not None:
            try:
                struct_type_raw = struct_type_field["object"]
                if int(struct_type_raw) != 0:
                    yield ("Type", _make_node_value(struct_type_raw))
                    return
            except Exception:
                pass
        yield from super()._yield_type()
    
    def _yield_type_specific(self):
        fields_field = find_field(self.impl, "fields")
        if fields_field is None:
            return
        try:
            fields_raw = fields_field["object"]
            if int(fields_raw) == 0:
                return
            dict_impl = resolve_impl(fields_raw)
            items = []
            for key_ptr, val_ptr in iter_dict(dict_impl):
                if int(key_ptr) == 0:
                    continue
                key_str = read_string_safe(key_ptr)
                if key_str is None:
                    continue
                items.append((key_str, _make_node_value(val_ptr)))
            if items:
                yield ("Fields", make_group(items, count_hint=len(items)))
        except Exception:
            pass


class ObjectNode(BaseNode):

    def _yield_type_specific(self):
        all_fields = self._get_all_fields()
        visible = [(n, v) for n, v in all_fields.items()
                   if n not in _HIDDEN_FIELDS]
        if visible:
            def make_members(items=visible):
                result = []
                for n, field_val in items:
                    child = make_child(n, field_val)
                    if child is not None:
                        result.append(child)
                return result
            yield ("Members", make_group(make_members,
                                        count_hint=len(visible)))


class StructTypeNode(BaseNode):

    def get_type_name(self):
        return "StructType"

    def get_display_value(self):
        type_name_field = find_field(self.impl, "typeName")
        if type_name_field is None:
            return None
        try:
            type_name_raw = type_name_field["object"]
            if int(type_name_raw) == 0:
                return None
            type_name = read_string_safe(type_name_raw)
            if type_name:
                return '"{}"'.format(type_name)
        except Exception:
            pass
        return None

    def _yield_type_specific(self):
        name_field = find_field(self.impl, "typeName")
        if name_field is not None:
            child = make_child("Name", name_field)
            if child is not None:
                yield child

        core_type = find_field(self.impl, "coreType")
        if core_type is not None:
            try:
                yield ("CoreType", make_leaf_value(_CORETYPE_NAMES.get(int(core_type), str(int(core_type)))))
            except Exception:
                yield ("CoreType", core_type)

        for field_name, display_name in (("names", "FieldNames"),
                                         ("defaultValues", "DefaultValues"),
                                         ("types", "FieldTypes")):
            field_val = find_field(self.impl, field_name)
            if field_val is None:
                continue
            child = make_child(display_name, field_val)
            if child is not None:
                yield child


class PropertyObjectNode(BaseNode):
    def get_type_name(self):
        return "PropertyObject"

    def _get_class_name(self):
        try:
            cn = find_field(self.impl, "className")
            if cn is not None:
                cn_raw = cn["object"]
                if int(cn_raw) != 0:
                    return read_string_safe(cn_raw)
        except Exception:
            pass
        return None

    def _yield_class(self):
        class_name = self._get_class_name()
        if class_name:
            yield ("Class", make_string_value(class_name))

    def _yield_properties(self):
        prop_pairs = list(get_all_properties(self.impl))
        def build_properties(pairs=prop_pairs):
            prop_list = []
            for prop_name, prop_ptr in pairs:
                try:
                    raw_obj = prop_ptr["object"]
                    if int(raw_obj) == 0:
                        continue
                    def make_prop_children(ptr=raw_obj):
                        try:
                            prop_impl = resolve_impl(ptr)
                            return list(get_generic_children(prop_impl))
                        except Exception:
                            return []
                    summary = get_property_summary(raw_obj)
                    if summary:
                        prop_list.append((prop_name,
                                         make_value_with_children(
                                             summary, make_prop_children)))
                    else:
                        prop_list.append((prop_name,
                                         make_group(make_prop_children)))
                except Exception:
                    continue
            return prop_list
        yield ("Properties", make_group(build_properties, count_hint=len(prop_pairs)))

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_properties()


class ComponentNode(PropertyObjectNode):

    def get_type_name(self):
        return "Component"

    def _yield_ids(self):
        for field_name, display_name in [("localId", "LocalId"),
                                         ("globalId", "GlobalId")]:
            fv = find_field(self.impl, field_name)
            if fv is None:
                continue
            try:
                raw = get_smart_ptr_raw(fv)
                if raw is None:
                    continue
                if int(raw) != 0:
                    yield (display_name, _make_node_value(raw))
                    continue
            except Exception:
                pass
            yield (display_name, fv)

    def _yield_attributes(self):
        present = [(f, d) for f, d in _COMPONENT_ATTRIBUTE_FIELDS
                   if find_field(self.impl, f) is not None]
        def build_attrs(items=present):
            result = []
            for field_name, display_name in items:
                fv = find_field(self.impl, field_name)
                if fv is None:
                    continue
                if field_name == "tags":
                    leaf = _get_tags_leaf(fv)
                    if leaf is not None:
                        result.append((display_name, leaf))
                    continue
                child = make_child(display_name, fv)
                if child is not None:
                    result.append(child)
            return result
        yield ("Attributes", make_group(build_attrs, count_hint=len(present)))

    def _read_map_key_text(self, key_val):
        try:
            vis = gdb.default_visualizer(key_val)
            if vis is not None:
                text = vis.to_string()
                if text is not None:
                    return str(text).strip('"')
        except Exception:
            pass
        try:
            return str(key_val)
        except Exception:
            return None

    def _read_component_local_id(self, raw_pointer):
        try:
            impl = resolve_impl(raw_pointer)
            local_id_field = find_field(impl, "localId")
            if local_id_field is None:
                return None
            local_id_raw = get_smart_ptr_raw(local_id_field)
            if local_id_raw is None:
                return None
            if int(local_id_raw) == 0:
                return None
            return read_string_safe(local_id_raw)
        except Exception:
            return None

    def _yield_component_field(self, field_name, display_name):
        fv = find_field(self.impl, field_name)
        if fv is None:
            return
        try:
            raw = get_smart_ptr_raw(fv)
            if raw is None:
                return
            if int(raw) != 0:
                yield (display_name, _make_node_value(raw))
        except Exception:
            return

    def _iter_folder_items(self, items_map):
        try:
            for pair in iter_ordered_map(items_map):
                try:
                    key_val = pair["first"]
                    comp_ptr = pair["second"]
                    comp_raw = comp_ptr["object"]
                    if int(comp_raw) == 0:
                        continue
                    local_id = self._read_component_local_id(comp_raw)
                    if local_id is None:
                        local_id = self._read_map_key_text(key_val)
                    if not local_id:
                        local_id = "?"
                    yield (local_id, comp_raw)
                except Exception:
                    continue
        except Exception:
            return

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_ids()
        yield from self._yield_attributes()
        yield from self._yield_properties()


class DeviceNode(ComponentNode):

    def get_type_name(self):
        return "Device"

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_ids()
        yield from self._yield_attributes()
        yield from self._yield_properties()
        yield from self._yield_component_field("signals", "Signals")
        yield from self._yield_component_field("functionBlocks", "FunctionBlocks")
        yield from self._yield_component_field("devices", "Devices")
        yield from self._yield_component_field("ioFolder", "IO")


class FolderNode(ComponentNode):

    def get_type_name(self):
        return "Folder"

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_ids()
        yield from self._yield_attributes()
        yield from self._yield_properties()
        items_map = find_field(self.impl, "items")
        if items_map is None:
            return
        raw_items = list(self._iter_folder_items(items_map))
        def make_items(items=raw_items):
            return [(lid, _make_node_value(rptr)) for lid, rptr in items]
        yield ("Items", make_group(make_items, count_hint=len(raw_items)))


class SignalNode(ComponentNode):

    def get_type_name(self):
        return "Signal"

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_ids()
        yield from self._yield_attributes()
        yield from self._yield_properties()
        is_pub = find_field(self.impl, "isPublic")
        if is_pub is not None:
            yield ("Public", make_leaf_value("true" if int(is_pub) else "false"))
        domain = find_field(self.impl, "domainSignal")
        if domain is not None:
            try:
                raw = get_smart_ptr_raw(domain)
                if raw is not None and int(raw) != 0:
                    yield ("DomainSignal", _make_node_value(raw))
            except Exception:
                pass

        for field_name, display_name in (("connections", "Connections"),
                                         ("remoteConnections", "RemoteConnections")):
            field_val = find_field(self.impl, field_name)
            if field_val is None:
                continue
            child = make_child(display_name, field_val)
            if child is not None:
                yield child


class InputPortNode(ComponentNode):

    def get_type_name(self):
        return "InputPort"

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_ids()
        yield from self._yield_attributes()
        yield from self._yield_properties()

        is_pub = find_field(self.impl, "isPublic")
        if is_pub is not None:
            yield ("Public", make_leaf_value("true" if int(is_pub) else "false"))

        requires_signal = find_field(self.impl, "requiresSignal")
        if requires_signal is not None:
            yield ("RequiresSignal", make_leaf_value("true" if int(requires_signal) else "false"))

        connection_ref = find_field(self.impl, "connectionRef")
        if connection_ref is not None:
            try:
                raw = get_smart_ptr_raw(connection_ref)
                if raw is not None and int(raw) != 0:
                    yield ("Connection", _make_node_value(raw))
            except Exception:
                pass


class FunctionBlockNode(ComponentNode):

    def get_type_name(self):
        return "FunctionBlock"

    def _yield_type_specific(self):
        yield from self._yield_class()
        yield from self._yield_ids()
        yield from self._yield_attributes()
        yield from self._yield_component_field("signals", "Signals")
        yield from self._yield_properties()
        yield from self._yield_component_field("functionBlocks", "FunctionBlocks")
        yield from self._yield_component_field("inputPorts", "InputPorts")


#MARK: END


def _get_tags_leaf(tags_ptr_field):
    try:
        raw = tags_ptr_field["object"]
        if int(raw) == 0:
            return make_leaf_value("[]")
        impl = resolve_impl(raw)
        tags_field = find_field(impl, "tags")
        if tags_field is None:
            return None
        items = []
        vis = gdb.default_visualizer(tags_field)
        if vis is not None:
            for _, item in vis.children():
                try:
                    inner_vis = gdb.default_visualizer(item)
                    if inner_vis is not None:
                        s = inner_vis.to_string()
                        items.append(s.strip('"') if s else str(item))
                    else:
                        items.append(str(item))
                except Exception:
                    items.append(str(item))
        return make_leaf_value("[{}]".format(", ".join(items)))
    except Exception:
        return None


def create_node(raw_pointer):
    if int(raw_pointer) == 0:
        return None

    impl = resolve_impl(raw_pointer)
    type_key = str(impl.type.strip_typedefs())

    if "StructTypeImpl" in type_key:
        return StructTypeNode(impl, raw_pointer)

    if "StatusContainerImpl" in type_key:
        return StatusContainerNode(impl, raw_pointer)

    category, type_name = classify_type(type_key)
    if category == "scalar":
        return ScalarNode(impl, raw_pointer, type_name)
    if category == "binary":
        return BinaryNode(impl, raw_pointer)
    if category == "enumeration":
        return EnumerationNode(impl, raw_pointer)
    if category == "struct":
        return StructNode(impl, raw_pointer)
    if category == "list":
        return ListNode(impl, raw_pointer)
    if category == "dict":
        return DictNode(impl, raw_pointer)
    if is_component(type_key, impl):
        impl_type = impl.type.strip_typedefs()
        type_leaf = type_key.split("::")[-1]
        if "Device" in type_leaf or _type_has(type_key, impl_type, "DeviceImpl"):
            return DeviceNode(impl, raw_pointer)
        if ("Signal" in type_leaf and "SignalContainer" not in type_leaf) or (_type_has(type_key, impl_type, "SignalImpl") and
                 not _type_has(type_key, impl_type, "SignalContainerImpl")):
            return SignalNode(impl, raw_pointer)
        if "InputPort" in type_leaf or _type_has(type_key, impl_type, "InputPortImpl"):
            return InputPortNode(impl, raw_pointer)
        if "FunctionBlock" in type_leaf or "Channel" in type_leaf or _type_has(type_key, impl_type, "FunctionBlockImpl"):
            return FunctionBlockNode(impl, raw_pointer)
        if "Folder" in type_leaf or "IoFolder" in type_leaf or _type_has(type_key, impl_type, "FolderImpl") or _type_has(type_key, impl_type, "IoFolderImpl"):
            return FolderNode(impl, raw_pointer)
        return ComponentNode(impl, raw_pointer)
    if find_field(impl, "localProperties") is not None:
        return PropertyObjectNode(impl, raw_pointer)
    if _hierarchy_contains(impl.type.strip_typedefs(), "GenericStructImpl"):
        return StructNode(impl, raw_pointer)
    return ObjectNode(impl, raw_pointer)


def _make_node_value(raw_ptr):
    if int(raw_ptr) == 0:
        return make_string_value("null")

    if _current_depth > _MAX_EXPANSION_DEPTH:
        deferred_ptr = raw_ptr
        try:
            preview = create_node(deferred_ptr)
            display = preview.to_string() if preview is not None else "{...}"
        except Exception:
            display = "{...}"
        def expand_deferred():
            try:
                node = create_node(deferred_ptr)
                if node is None:
                    return []
                return list(node.children())
            except Exception:
                return []
        gid = _next_id()
        _synthetic_storage[gid] = (gid, display, expand_deferred, _current_depth)
        return _alloc_void_ptr(gid)

    try:
        node = create_node(raw_ptr)
        if node is None:
            return make_string_value("0x{:x}".format(int(raw_ptr)))
        display = node.to_string()
        def make_children(n=node):
            return list(n.children())
        return make_value_with_children(display, make_children)
    except Exception:
        return make_string_value("0x{:x}".format(int(raw_ptr)))



class OpenDaqPrinter:
    def __init__(self, value, is_smart_pointer=False, type_name=""):
        self.value = value
        self.is_smart_pointer = is_smart_pointer
        self.type_name = type_name
        self._node = None
        self._resolved = False

    def _yield_smart_pointer_children(self):
        try:
            yield ("Borrowed", self.value["borrowed"])
        except Exception:
            pass

        try:
            raw = self._get_raw_pointer()
            if raw is None:
                yield ("Object", make_string_value("null"))
            else:
                yield ("Object", raw)
        except Exception:
            pass

        try:
            node = self._get_node()
            if isinstance(node, FolderNode):
                for name, val in node.children():
                    if name == "Items":
                        yield (name, val)
                        break
        except Exception:
            pass

    def _get_raw_pointer(self):
        if self.is_smart_pointer:
            return get_smart_ptr_raw(self.value)
        return self.value

    def _get_node(self):
        if not self._resolved:
            self._resolved = True
            try:
                raw = self._get_raw_pointer()
                if raw is not None and int(raw) != 0:
                    self._node = create_node(raw)
            except Exception:
                pass
        return self._node

    def to_string(self):
        try:
            raw = self._get_raw_pointer()
            if raw is None or int(raw) == 0:
                return "null"
            node = self._get_node()
            if node is not None:
                return node.to_string()
            return "0x{:x}".format(int(raw))
        except Exception:
            return str(self.value)

    def children(self):
        global _current_depth
        try:
            saved_depth = _current_depth
            _current_depth = 1
            try:
                if self.is_smart_pointer:
                    children_list = list(self._yield_smart_pointer_children())
                else:
                    node = self._get_node()
                    children_list = list(node.children()) if node is not None else []
            finally:
                _current_depth = saved_depth
            for c in children_list:
                yield c
        except Exception:
            return


class OpenDaqPrinterLookup:    
    name = "openDAQ"
    enabled = True
    subprinters = []
    
    def __call__(self, value):
        try:
            type_name = str(value.type.strip_typedefs())
        except Exception:
            return None
        
        if "Ptr" in type_name:
            try:
                value["borrowed"]
                value["object"]
                result = OpenDaqPrinter(value, is_smart_pointer=True, type_name=type_name)
                return result
            except Exception:
                pass
        
        if type_name == "unsigned long long":
            try:
                addr = int(value)
                if addr in _leaf_storage:
                    result = LeafPrinter(_leaf_storage[addr])
                    return result
                if addr in _string_storage:
                    result = StringPrinter(_string_storage[addr])
                    return result
            except Exception:
                pass
            return None

        if type_name == "void *":
            try:
                addr = int(value)
                if addr in _synthetic_storage:
                    gid, text, source, depth = _synthetic_storage[addr]
                    result = SyntheticPrinter(gid, text, source, depth)
                    return result
            except Exception:
                pass
            return None

        if "daq::" not in type_name:
            return None

        for _container_prefix in ("std::", "tsl::", "boost::"):
            if type_name.startswith(_container_prefix):
                return None

        
        if type_name.endswith("*") and "::I" in type_name:
            if int(value) != 0:
                result = OpenDaqPrinter(value, is_smart_pointer=False, type_name=type_name)
                return result
        
        return None
    

gdb.printing.register_pretty_printer(gdb, OpenDaqPrinterLookup(), replace=True)

gdb.events.new_objfile.connect(
    lambda event: gdb.printing.register_pretty_printer(event.new_objfile, OpenDaqPrinterLookup(), replace=True)
)

def on_continue(event):
    global _group_id_counter
    _string_storage.clear()
    _leaf_storage.clear()
    _synthetic_storage.clear()
    _synthetic_children_cache.clear()
    _dynamic_type_by_addr.clear()
    _group_id_counter = 0

try:
    gdb.events.cont.connect(on_continue)
except AttributeError:
    pass



import time as _time

_PROFILE_STATS = {}


def _profiled(func, name=None):
    label = name or func.__name__
    _PROFILE_STATS.setdefault(label, [0, 0.0])

    def wrapper(*args, **kwargs):
        t0 = _time.perf_counter()
        try:
            return func(*args, **kwargs)
        finally:
            elapsed = _time.perf_counter() - t0
            entry = _PROFILE_STATS[label]
            entry[0] += 1
            entry[1] += elapsed
    wrapper.__name__ = getattr(func, "__name__", label)
    return wrapper


def reset_profile():
    for k in list(_PROFILE_STATS.keys()):
        _PROFILE_STATS[k] = [0, 0.0]


def print_profile():
    rows = [(name, c, t) for name, (c, t) in _PROFILE_STATS.items() if c > 0]
    rows.sort(key=lambda r: -r[2])
    print("--- openDAQ printer profile ---")
    print("{:<42} {:>10} {:>12} {:>14}".format(
        "function", "calls", "total (s)", "avg (us)"))
    for name, count, total in rows:
        avg_us = (total / count * 1e6) if count else 0.0
        print("{:<42} {:>10} {:>12.4f} {:>14.2f}".format(
            name, count, total, avg_us))
    print("-------------------------------")


for _fn_name in (
    "resolve_impl",
    "get_dynamic_type",
    "find_field",
    "collect_field_values",
    "_make_node_value",
    "create_node",
    "iter_dict",
    "iter_deque",
    "iter_ordered_map",
    "iter_local_properties",
    "iter_class_properties",
    "get_all_properties",
    "read_string_safe",
    "get_smart_ptr_raw",
    "get_property_summary",
    "make_child",
    "get_generic_children",
):
    _orig = globals().get(_fn_name)
    if _orig is not None:
        globals()[_fn_name] = _profiled(_orig, _fn_name)


BaseNode.children = _profiled(BaseNode.children, "BaseNode.children")
BaseNode.to_string = _profiled(BaseNode.to_string, "BaseNode.to_string")
BaseNode._get_all_fields = _profiled(BaseNode._get_all_fields, "BaseNode._get_all_fields")
ListNode._setup = _profiled(ListNode._setup, "ListNode._setup")
DictNode._setup = _profiled(DictNode._setup, "DictNode._setup")
FolderNode._iter_folder_items = _profiled(FolderNode._iter_folder_items, "FolderNode._iter_folder_items")
OpenDaqPrinter.to_string = _profiled(OpenDaqPrinter.to_string, "OpenDaqPrinter.to_string")
OpenDaqPrinter.children = _profiled(OpenDaqPrinter.children, "OpenDaqPrinter.children")
OpenDaqPrinterLookup.__call__ = _profiled(OpenDaqPrinterLookup.__call__, "OpenDaqPrinterLookup.__call__")


class _OpenDaqProfileCommand(gdb.Command):
    """opendaq-profile [reset]  -- print or reset the openDAQ printer profile."""

    def __init__(self):
        super().__init__("opendaq-profile", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        arg = (arg or "").strip().lower()
        if arg == "reset":
            reset_profile()
            print("openDAQ profile reset.")
        else:
            print_profile()


try:
    _OpenDaqProfileCommand()
except Exception as _e:
    print("Could not register opendaq-profile command: {}".format(_e))
