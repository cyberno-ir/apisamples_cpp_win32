#include "json.hpp"

namespace json {

	JSON Object() {
		return std::move(JSON::Make(JSON::Class::Object));
	}

	JSON Array() {
		return std::move(JSON::Make(JSON::Class::Array));
	}

	JSON JSON::Load(const string &str) {
		size_t offset = 0;
		return std::move(parse_next(str, offset));
	}
}
