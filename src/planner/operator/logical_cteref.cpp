#include "duckdb/planner/operator/logical_cteref.hpp"

#include "duckdb/main/config.hpp"

namespace duckdb {

case_insensitive_map_t<string> LogicalCTERef::ParamsToString() const {
	case_insensitive_map_t<string> result;
	result["CTE Index"] = StringUtil::Format("%llu", cte_index);
	return result;
}

vector<idx_t> LogicalCTERef::GetTableIndex() const {
	return vector<idx_t> {table_index};
}

string LogicalCTERef::GetName() const {
#ifdef DEBUG
	if (DBConfigOptions::debug_print_bindings) {
		return LogicalOperator::GetName() + StringUtil::Format(" #%llu", table_index);
	}
#endif
	return LogicalOperator::GetName();
}

} // namespace duckdb
