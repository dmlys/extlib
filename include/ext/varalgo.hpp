#pragma once

/// wrappers of standard algorithms,
/// those wrappers allows to pass boost::variant<Pred1, Pred2, ...> in place of sort predicate
/// that variant will be dispatched and algorithm will be called with actual predicate

#include <ext/varalgo/sorting_algo.hpp>
#include <ext/varalgo/on_sorted_algo.hpp>
#include <ext/varalgo/set_operations.hpp>

#include <ext/varalgo/minmax.hpp>
#include <ext/varalgo/minmax_element.hpp>

#include <ext/varalgo/partition_algo.hpp>

#include <ext/varalgo/non-modifying_algo.hpp>
#include <ext/varalgo/modifying_algo.hpp>
#include <ext/varalgo/for_each.hpp>
