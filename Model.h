﻿#pragma once

#include <iostream>
#include <string>
#include <assert.h>

#include "SqlBuilder.h"
#include "nanodbc/nanodbc.h"

/// \brief acts as an identifying base class for generated model classes and contains
/// static binding and SQL operations.  Should never have non-static methods/members
/// to maintain a minimal footprint
class Model
{
	// Mostly hand-coded template with some possible generated functions/properties.
public:
	template <typename T>
	using BindingIndex = std::vector<typename T::BinderType::BindColumnFunction_t>;

	/// \brief uses a SqlBuilder to select a batch of data in a single trip to the database
	///
	/// \param sql SqlBuilder used to modify the select query
	/// \note this currently acts as an iterator and is not efficient
	/// \see ModelRecordSet for efficient iterator implementation until this is done correctly
	template <typename T>
	static std::vector<T> BatchSelect(SqlBuilder<T>& sql) noexcept(false)
	{
		nanodbc::connection conn(DatabaseConnManager::GetConnectionTo(T::DbType()));

		std::string query = sql.SelectCountString();

		nanodbc::statement stmt = nanodbc::statement(conn, query);
		nanodbc::result result = nanodbc::execute(stmt);
		int64_t rowCount = 0;

		if (result.next())
			rowCount = result.get<int64_t>(0);

		query = sql.SelectString();
		stmt = nanodbc::statement(conn, query);
		result = nanodbc::execute(stmt);

		BindingIndex<T> bindingsIndex;
		IndexColumnNameBindings<T>(result, bindingsIndex);

		std::vector<T> resultModels;
		if (rowCount > 0)
			resultModels.reserve(static_cast<size_t>(rowCount));

		// result always starts before the first row, so calling next will not skip the first result.
		while (result.next())
		{
			T res {};
			Model::BindResult(result, res, bindingsIndex);
			resultModels.push_back(
				std::move(res));
		}

		return resultModels;
	}

	/// \brief uses a SqlBuilder to select a batch of data in a single trip to the database
	///
	/// \note this currently acts as an iterator and is not efficient
	/// \see ModelRecordSet for efficient iterator implementation until this is done correctly
	template <typename T>
	static std::vector<T> BatchSelect() noexcept(false)
	{
		SqlBuilder<Item> filter {};
		return std::move(
			BatchSelect(filter));
	}

	/// \brief attempts to bind a result row to a given model class's members
	template <typename T>
	static void BindResult(const nanodbc::result& result, T& model, const BindingIndex<T>& bindingIndex)
	{
		// This is managed externally so we shouldn't really have to verify in release builds.
		// But we should still ensure that it's setup appropriately in debug builds.
		assert(result.columns() == bindingIndex.size());

		for (short i = 0; i < result.columns(); i++)
		{
			auto bind = bindingIndex[i];
			if (bind != nullptr)
				bind(model, result, i);
		}
	}

	template <typename T>
	static void IndexColumnNameBindings(const nanodbc::result& result, BindingIndex<T>& bindingsIndex)
	{
		using BinderType = typename T::BinderType;

		const auto& columnBindingsMap = BinderType::GetColumnBindings();
		std::string columnName;

		bindingsIndex.reserve(result.columns());

		for (short i = 0; i < result.columns(); i++)
		{
			columnName = result.column_name(i);

			auto itr = columnBindingsMap.find(columnName);
			if (itr == columnBindingsMap.end())
			{
				// TODO: logger impl
				std::cout << "WARN: No binding found for:" << T::TableName() << "." << columnName << "\n";
				bindingsIndex.push_back(nullptr);
				continue;
			}

			bindingsIndex.push_back(itr->second);
		}
	}
};
