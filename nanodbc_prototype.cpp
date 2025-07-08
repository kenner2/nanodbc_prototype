#include <iostream>
#include <string>
#include <optional>

#include "DatabaseConnManager.h"
#include "Item.h"
#include "ModelRecordSet.h"
#include "SqlBuilder.h"
#include "nanodbc/nanodbc.h"

import FullModel;

int main()
{
	try
	{
		std::string dsn = "KN_online";
		std::string user = "knight";
		std::string password = "knight";
		DatabaseConnManager::SetGameDsn(dsn, user, password);
		
		SqlBuilder<model::Item> filter {};
		filter.SetSelectColumns({ "Num", "strName" });

		// using an iterator
		ModelRecordSet<model::Item> recordSet(filter);
		int i = 0;
		while (recordSet.next() && i < 10)
		{
			model::Item item = recordSet.get();
			std::cout << item.Number << ": " << item.Name << "\n";
			i++;
		}

		// loading results in a single query; currently this functions the same way as the iterator above
		// need to look more at nanodbc library functions to see if there's a way to fetch everything
		// in one database trip or not
		std::vector<model::Item> results = Model::BatchSelect<model::Item>(filter);
		for (std::size_t ix = 0; ix < results.size() && ix < 10; ix++)
		{
			std::cout << results.at(ix).Number << ": " << results.at(ix).Name << "\n";
		}
	}
	catch (nanodbc::database_error& ex)
	{
		std::cerr << ex.what() << "\n";
	}
}
