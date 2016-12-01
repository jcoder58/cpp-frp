/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _FRP_STATIC_PUSH_FILTER_H_
#define _FRP_STATIC_PUSH_FILTER_H_

#include <frp/internal/namespace_alias.h>
#include <frp/static/push/repository.h>
#include <frp/util/collector.h>
#include <frp/vector_view.h>
#include <vector>

namespace frp {
namespace stat {
namespace push {

template<typename Comparator, typename Function, typename Dependency>
auto filter(Function &&function, Dependency &&dependency) {
	typedef typename util::unwrap_reference_t<Dependency>::value_type::value_type value_type;
	static_assert(!std::is_void<value_type>::value, "T must not be void type.");
	static_assert(std::is_copy_constructible<value_type>::value, "T must be copy constructible.");
	typedef vector_view_type<value_type, Comparator> collector_view_type;
	typedef util::commit_storage_type<collector_view_type, 1> commit_storage_type;
	return details::make_repository<collector_view_type, commit_storage_type,
		std::equal_to<collector_view_type>>([
			function = std::move(internal::get_function(function)),
			executor = std::move(internal::get_executor(function))](
			auto &&callback, const auto &previous, const auto & storage) {
		typedef std::array<util::revision_type, 1> revisions_type;
		typedef util::append_collector_type<value_type, Comparator> collector_type;
		if (storage->value.empty()) {
			callback(std::make_shared<commit_storage_type>(collector_view_type(collector_type(0)),
				util::default_revision, revisions_type{ storage->revision }));
		}
		else {
			auto collector(std::make_shared<collector_type>(storage->value.size()));
			std::size_t counter(0);
			for (const auto &value : storage->value) {
				std::size_t index(counter++);
				executor([function, storage, collector, index, &value, callback]() {
					if (function(value)
						? collector->construct(std::ref(value)) : collector->skip()) {
						callback(std::make_shared<commit_storage_type>(
							collector_view_type(std::move(*collector)),
							util::default_revision, revisions_type{ storage->revision }));
					}
				});
			}
		}
	}, std::forward<Dependency>(dependency));
}

template<typename Function, typename Dependency>
auto filter(Function &&function, Dependency &&dependency) {
	typedef typename util::unwrap_reference_t<Dependency>::value_type::value_type value_type;
	static_assert(util::is_equality_comparable<value_type>::value,
		"T must implement equality comparator");
	return filter<std::equal_to<value_type>>(std::forward<Function>(function),
		std::forward<Dependency>(dependency));
}

} // namespace push
} // namespace stat
} // namespace frp

#endif // _FRP_STATIC_PUSH_FILTER_H_
