/*
    nanobind/stl/bind_map.h: Automatic creation of bindings for map-style containers

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/detail/traits.h>

NAMESPACE_BEGIN(NB_NAMESPACE)

template <typename Map, typename... Args>
class_<Map> bind_map(handle scope, const char *name, Args &&...args) {
    using Key = typename Map::key_type;
    using Value = typename Map::mapped_type;

    auto cl = class_<Map>(scope, name, std::forward<Args>(args)...)
        .def(init<>())

        .def("__len__", &Map::size)

        .def("__bool__",
             [](const Map &m) { return !m.empty(); },
             "Check whether the map is nonempty")

        .def("__contains__",
             [](const Map &m, const Key &k) { return m.find(k) != m.end(); })

        .def("__contains__", // fallback for incompatible types
             [](const Map &, handle) { return false; })

        .def("__iter__",
             [](Map &m) {
                 return make_key_iterator(type<Map>(), "KeyIterator",
                                          m.begin(), m.end());
             },
             keep_alive<0, 1>())

        .def("__getitem__",
             [](Map &m, const Key &k) -> Value & {
                 auto it = m.find(k);
                 if (it == m.end())
                     throw key_error();
                 return it->second;
             },
             rv_policy::reference_internal
        )

        .def("__delitem__",
            [](Map &m, const Key &k) {
                auto it = m.find(k);
                if (it == m.end())
                    throw key_error();
                m.erase(it);
            }
        );

    // Assignment operator for copy-assignable/copy-constructible types
    if constexpr (detail::is_copy_assignable_v<Value> ||
                  detail::is_copy_constructible_v<Value>) {
        cl.def("__setitem__", [](Map &m, const Key &k, const Value &v) {
            if constexpr (detail::is_copy_assignable_v<Value>) {
                m[k] = v;
            } else {
                auto r = m.emplace(k, v);
                if (!r.second) {
                    // Value is not copy-assignable. Erase and retry
                    m.erase(r.first);
                    m.emplace(k, v);
                }
            }
        });
    }

    // Item, value, and key views
    struct KeyView   { Map &map; };
    struct ValueView { Map &map; };
    struct ItemView  { Map &map; };

    class_<ItemView>(cl, "ItemView")
        .def("__len__", [](ItemView &v) { return v.map.size(); })
        .def("__iter__",
             [](ItemView &v) {
                 return make_iterator(type<Map>(), "ItemIterator",
                                      v.map.begin(), v.map.end());
             },
             keep_alive<0, 1>());

    class_<KeyView>(cl, "KeyView")
        .def("__contains__", [](KeyView &v, const Key &k) { return v.map.find(k) != v.map.end(); })
        .def("__contains__", [](KeyView &, handle) { return false; })
        .def("__len__", [](KeyView &v) { return v.map.size(); })
        .def("__iter__",
             [](KeyView &v) {
                 return make_key_iterator(type<Map>(), "KeyIterator",
                                          v.map.begin(), v.map.end());
             },
             keep_alive<0, 1>());

    class_<ValueView>(cl, "ValueView")
        .def("__len__", [](ValueView &v) { return v.map.size(); })
        .def("__iter__",
             [](ValueView &v) {
                 return make_value_iterator(type<Map>(), "ValueIterator",
                                            v.map.begin(), v.map.end());
             },
             keep_alive<0, 1>());

    cl.def("keys", [](Map &m)   { return new KeyView{m}; }, keep_alive<0, 1>());
    cl.def("values", [](Map &m) { return new ValueView{m}; }, keep_alive<0, 1>());
    cl.def("items", [](Map &m)  { return new ItemView{m}; }, keep_alive<0, 1>());

    return cl;
}

NAMESPACE_END(NB_NAMESPACE)
